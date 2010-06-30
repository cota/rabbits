#include <cfg.h>
#include <etrace_if.h>

#ifdef ENERGY_TRACE_ENABLED

#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

static int      s_pid_graph[20];
static int      s_nb_graph = 0;

static void close_graph ()
{
    int         i, status;

    if (s_nb_graph == 0)
        return;

    for (i = 0; i < s_nb_graph; i++)
    {
        kill (s_pid_graph[i], SIGKILL);
        ::wait (&status);
    }

    s_nb_graph = 0;
}

sample_t *
sample_new (scope_state_t *scope, uint64_t prev_stamp)
{
    sample_t    *new_samp = new sample_t;
    int         i;

    if (new_samp == NULL)
    {
        cerr << "Run out of memory" << endl;
        return NULL;
    }

    new_samp->next       = NULL;
    new_samp->time_stamp = prev_stamp + scope->sample_period;

    for (i = 0; i < scope->nb_periphs; i++)
    {
        new_samp->per_costs[i].processed = 0;
        new_samp->per_costs[i].energy    = 0;
    }

    return new_samp;
}

void
sample_free (sample_t *sample)
{
    delete (sample);
}

sample_t *
__r_update_samples (scope_state_t *scope, sample_t *sample, 
    periph_t *pperiph, int per_mode, uint64_t time, uint64_t last_time)
{
    if (time > last_time)
    { 
        int      curr_mode   = pperiph->curr_mode;
        int      curr_fvset  = pperiph->curr_fvset;
        int      n_modes     = pperiph->pclass->nmodes;
        uint64_t mode_cost   =
            pperiph->pclass->mode_cost[n_modes * curr_fvset + curr_mode];
        uint64_t last_change = pperiph->last_change_stamp;

        if (sample == NULL)
        { 
            // if end of list create a new sample
            sample = sample_new (scope, last_time);
            if (sample == NULL)
                return NULL;
        }

        // now it is not useful to update this sample for this periph anymore
        sample->per_costs[pperiph->idx].processed = 1;

        if (sample->time_stamp < last_change)
        {
            // already processed ... with previous events.
            sample->next = __r_update_samples (scope, sample->next, pperiph,
                per_mode, time, sample->time_stamp);
        }
        else
        {
            // not processed
            uint64_t begin_stamp = 0;
            uint64_t end_stamp   = 0;
            if (last_time < last_change)
                begin_stamp = last_change;  // or not completely processed
            else
                begin_stamp = last_time;
            
            if (time > sample->time_stamp)
                end_stamp = sample->time_stamp;
            else
                end_stamp = time;

            sample->per_costs[pperiph->idx].energy  += (end_stamp - begin_stamp) * mode_cost;

            if (time > sample->time_stamp) // recursion ?
                sample->next = __r_update_samples (scope, sample->next, pperiph,
                    per_mode, time, sample->time_stamp);
        }
    }

    return sample;
}

void
update_samples (scope_state_t *scope, periph_t *pperiph, int per_mode, uint64_t time_stamp)
{
    scope->samples_head =
    __r_update_samples (scope, scope->samples_head, pperiph,
        per_mode, time_stamp, scope->last_measure_stamp);
}

sample_t *
get_sample (scope_state_t *scope, uint64_t time)
{
    sample_t    *samp = scope->samples_head;

    if (samp == NULL)
        samp = sample_new (scope, scope->last_measure_stamp);

    if (samp->time_stamp != time) //error
        return NULL;

    periph_t    *pperiph;
    for (pperiph = scope->head_periphs; pperiph != NULL; pperiph = pperiph->next)
    {
        int      curr_mode         = pperiph->curr_mode;
        int      curr_fvset        = pperiph->curr_fvset;
        int      n_modes           = pperiph->pclass->nmodes;
        uint64_t curr_mode_cost    =
            pperiph->pclass->mode_cost[curr_fvset * n_modes + curr_mode];
        uint64_t last_change_stamp = pperiph->last_change_stamp;
        uint64_t update_en         = 0;

        if (((time - last_change_stamp) >= 0) &&
            ((time - last_change_stamp) < scope->sample_period))
        {
            // last mode change occured in this sample ... update the end of sample
            update_en = curr_mode_cost * (time - last_change_stamp);
        }
        else
        {
            if (samp->per_costs[pperiph->idx].processed != 1)
                update_en = curr_mode_cost * (scope->sample_period);
        }

        samp->per_costs[pperiph->idx].energy += update_en;
    }

    scope->samples_head       = samp->next;
    scope->last_measure_stamp = samp->time_stamp;

    return samp;
}

void
etrace_if_mode_change (scope_state_t *scope, periph_t *pperiph, unsigned long mode,
    unsigned long long time_stamp)
{

    int new_mode  = mode & 0x1F;
    int new_fvset = ((mode >> 5) & 0x07);

    // check the new mode and new fvset (do they exist)
    if (new_mode >= pperiph->pclass->nmodes)
    {
        cerr << "Wrong mode (" << new_mode << " for peripheral : " <<
            pperiph->name << ")" << endl;
        return;
    }

    if (new_fvset >= pperiph->pclass->nfvsets)
    {
        cerr << "Wrong FV set (" << new_fvset << " for peripheral index : " <<
            pperiph->name << ")" << endl;
        return;
    }

    // check if the time_stamp is not before last_evt time stamp
    if (time_stamp < pperiph->last_change_stamp)
    {
        cerr << "Time event not correct : new event time : "
        << time_stamp << " whereas previous event was at "
        << pperiph->last_change_stamp << endl;
        return;
    }

    // update samples if needed
    if (time_stamp > pperiph->last_change_stamp)
        update_samples (scope, pperiph, mode, time_stamp);

    // update periph status
    pperiph->curr_mode         = new_mode;
    pperiph->curr_fvset        = new_fvset;
    pperiph->last_change_stamp = time_stamp;
}

// ===========================
// public class functions
// ===========================

static inline void writepipe (int &pipe, const void* adr, int nbytes)
{
    if (pipe != 0 && nbytes != ::write (pipe, adr, nbytes))
        pipe = 0;
}

void
etrace_if::display_init (void)
{
    int             i;
    int             disp_pipe[2];
    unsigned int    val;
    long long       long_val;
    const char      *pbuf;

    if (pipe (disp_pipe) < 0)
    {
        perror ("etrace_if: pipe error");
        exit (EXIT_FAILURE);
    }
  
    if ((s_pid_graph[s_nb_graph++] = fork ()) == 0)
    {
        setpgrp();

        // son
        int         null_fd;

        dup2 (disp_pipe[0], STDIN_FILENO);
        close (disp_pipe[0]);
        close (disp_pipe[1]);

        null_fd = open ("/dev/null", O_WRONLY);
        // make it silent
        dup2 (null_fd, STDOUT_FILENO);
        //dup2 (null_fd, STDERR_FILENO);
        close (null_fd);

        if (execlp ("chronograph", "chronograph", NULL) < 0)
        {
            perror ("etrace_if: execlp failure");
            _exit (EXIT_FAILURE);
        }
    }
    
    // father
    signal (SIGPIPE, SIG_IGN);
    close (disp_pipe[0]);
    m_disp_pipe = disp_pipe[1];

    // setting the graph parameters
    pbuf = "Power consumption";
    val  = strlen (pbuf) + 1;
    writepipe (m_disp_pipe, &val, 4);
    writepipe (m_disp_pipe, pbuf, val);

    // number of graphs
    val = m_scope->nb_groups + 1;
    writepipe (m_disp_pipe, &val, 4);

    // sample period (20 ms)
    val = 20;
    writepipe (m_disp_pipe, &val, 4);

    // for each graph: number of curves, min value,  max value
    int         j, group_id;
    periph_t    *tp, *pperiph = m_scope->head_periphs;

    i = 0;
    while (pperiph)
    {
        group_id = pperiph->group_id;

        tp = pperiph;
        j = i;
        while (tp && tp->group_id == group_id)
        {
            tp->idx = j++;
            tp = tp->next;
        }

        // number of curves in the graph
        val = j - i;
        writepipe (m_disp_pipe, &val, 4);

        // min value
        long_val = 0;
        writepipe (m_disp_pipe, &long_val, 8);

        // max value 
        long_val = pperiph->pclass->max_energy_ms;
        m_scope->max_energy_ms += (j - i) * long_val;
        writepipe (m_disp_pipe, &long_val, 8);
        
        i = j;
        pperiph = tp;
    }

    // for total graph
    // number of curves in the graph (2)
    val = 2;
    writepipe (m_disp_pipe, &val, 4);

    // min value
    long_val = 0;
    writepipe (m_disp_pipe, &long_val, 8);

    // max value 
    long_val =m_scope->max_energy_ms;
    writepipe (m_disp_pipe, &long_val, 8);

    // for each graph: graphs names
    pperiph = m_scope->head_periphs;
    while (pperiph)
    {
        pbuf = pperiph->group_name;
        val  = strlen (pbuf) + 1;
        writepipe (m_disp_pipe, &val, 4);
        writepipe (m_disp_pipe, pbuf, val);

        group_id = pperiph->group_id;
        while (pperiph && pperiph->group_id == group_id)
            pperiph = pperiph->next;
    }

    // for total graph
    pbuf = "Total ";
    val  = strlen (pbuf) + 1;
    writepipe (m_disp_pipe, &val, 4);
    writepipe (m_disp_pipe, pbuf, val);
}

etrace_if::etrace_if (sc_module_name name)
  :sc_module (name)
{
    // init scope C things
    m_scope = new scope_state_t;
    memset (m_scope, 0, sizeof (scope_state_t));
    m_scope->sample_period      = 20 * 1000 * 1000; // 100 ms

    m_full_average = 0;
    m_curr_idx     = 0;

    m_measure_started = 0;
    m_measure_accum   = 0;

    atexit (close_graph);

    // init SystemC stuffs.
    SC_THREAD (scope_trigger);
}


etrace_if::~etrace_if (void)
{
    delete m_scope;
}

void
etrace_if::scope_trigger (void)
{
    if (m_scope->nb_periphs == 0)
        return;

    // init display
    display_init ();

    uint64_t                sum, samp_per;
    sample_t                *samp;
    periph_t                *pperiph;
    uint8_t                 *vals;


    vals = new uint8_t[m_scope->nb_periphs + 2];
    samp_per = m_scope->sample_period;

    while (1)
    {
        // sample rate ... and be sure to be last evaluated
        wait (m_scope->sample_period, SC_NS);
        wait (0, SC_NS);

        // get current sample
        samp = get_sample (m_scope, sc_time_stamp ().value () / 1000);

        // process sample (according to rules given by user -- or not
        sum = 0;
        for (pperiph = m_scope->head_periphs; pperiph != NULL; pperiph = pperiph->next)
        {
            sum += samp->per_costs[pperiph->idx].energy;
            vals[pperiph->idx] =
                (uint8_t) ((samp->per_costs[pperiph->idx].energy * 100) /
                (samp_per * pperiph->pclass->max_energy_ms));
        }

        vals[m_scope->nb_periphs] = 
            (uint8_t) ((sum * 100) / (samp_per * m_scope->max_energy_ms));
        vals[m_scope->nb_periphs + 1] =
            (uint8_t) ((roll_average (sum) * 100) / (samp_per * m_scope->max_energy_ms));

        if (m_measure_started)
        {
            m_measure_accum += sum;
            m_measure_nbsamp++;
        }

        // display the values
        writepipe (m_disp_pipe, vals, m_scope->nb_periphs + 2);

        // clean things up
        if (samp != NULL)
            sample_free (samp);
    }
}

unsigned long etrace_if::add_periph (const char *name,
    const periph_class_t *pclass, int &group_id, const char * group_name)
{
    periph_t    *pperiph = new periph_t;
    memset (pperiph, 0, sizeof (periph_t));

    pperiph->pclass = pclass;
    pperiph->name = strdup (name);
    pperiph->group_name = strdup (group_name);

    m_scope->nb_periphs++;
    if (group_id < 0)
        group_id = m_scope->nb_groups++;
    pperiph->group_id = group_id;

    periph_t    **tp = &m_scope->head_periphs;
    while (*tp && group_id >= (*tp)->group_id)
        tp = & (*tp)->next;

    pperiph->next = *tp;
    *tp = pperiph;

    return (unsigned long) pperiph;
}

uint64_t
etrace_if::roll_average (uint64_t new_samp)
{
    int         i = 0;
    uint64_t    acc = 0;
    uint64_t    res = 0;

    m_history[m_curr_idx] = new_samp;

    if (m_full_average)
    {
        for (i = 0; i < MAX_HISTORY; i++)
            acc += m_history[i];

        res = acc / MAX_HISTORY;
    }
    else
    {
        for (i = 0; i <= m_curr_idx; i++)
            acc += m_history[i];

        res = acc / (m_curr_idx + 1);
    }

    // update current index for next call
    m_curr_idx = (m_curr_idx + 1) % MAX_HISTORY;
    if (m_curr_idx == (MAX_HISTORY - 1))
        m_full_average = 1;

    return res;
}


void 
etrace_if::change_energy_mode (unsigned long periph_id, unsigned long mode)
{
    etrace_if_mode_change (m_scope, 
        (periph_t *) periph_id, mode, sc_time_stamp ().value () / 1000);
}

void 
etrace_if::energy_event (unsigned long periph_id, unsigned long event_id, unsigned long value)
{
}

void 
etrace_if::start_measure (void)
{
    m_measure_started = 1;
    m_measure_accum   = 0;
    m_measure_nbsamp  = 0;
}
 
unsigned long
etrace_if::stop_measure (void)
{
    uint64_t  samp_per    = m_scope->sample_period;
    uint64_t  val         = m_measure_accum / samp_per / m_measure_nbsamp;

    m_measure_started = 0;

    return (unsigned long) val;
}

etrace_if       etrace ("Etrace_Interface");

#endif
