#ifndef _ETRACE_IF_H_
#define _ETRACE_IF_H_

#include <cfg.h>

#ifdef ENERGY_TRACE_ENABLED

#include <systemc.h>
#include <stdint.h>

#define ETRACE_MODE(mode, fvset)    (((fvset) << 5) | (mode))
#define ETRACE_CPU_IDLE         0
#define ETRACE_CPU_RUNNING      1

#define MAX_PERIPHERALS         20
#define MAX_HISTORY             10

#define WRITE_COMMAND           0x0
#define READ_COMMAND            0x1

typedef struct sample           sample_t;
typedef struct per_en           per_en_t;
typedef struct scope_state      scope_state_t;
typedef struct periph           periph_t;

typedef struct
{
    const char                  *class_name;
    int                         max_energy_ms;
    int                         nmodes;
    int                         nfvsets;
    uint64_t                    *mode_cost;
} periph_class_t;

struct periph
{
    char                        *name;
    const periph_class_t        *pclass;
    int                         group_id;
    char                        *group_name;
    periph_t                    *next;
    int                         idx;

    uint64_t                    last_change_stamp;
    int                         curr_mode;
    int                         curr_fvset;
};

struct scope_state
{
    uint64_t                    sample_period;
    sample_t                    *samples_head;
    uint64_t                    last_measure_stamp;
    periph_t                    *head_periphs;
    int                         nb_periphs;
    int                         nb_groups;
    int                         max_energy_ms;
};

struct per_en
{
    int                         processed;
    uint64_t                    energy;
};

struct sample
{
    sample_t                    *next;
    uint64_t                    time_stamp;
    per_en_t                    per_costs[MAX_PERIPHERALS];
};

class etrace_if : public sc_module
{
public:
    SC_HAS_PROCESS (etrace_if);
    etrace_if (sc_module_name name);
    ~etrace_if ();

public:
    unsigned long add_periph (const char *name, const periph_class_t *pclass, int &group_id,
        const char * group_name);
    void change_energy_mode (unsigned long periph_id, unsigned long mode);
    void energy_event (unsigned long periph_id, unsigned long event_id, unsigned long value);

	void start_measure (void);
	unsigned long stop_measure (void);

private:
    void display_init (void);
    void scope_trigger (void);
    uint64_t roll_average (uint64_t new_samp);

private:
    scope_state_t               *m_scope;
    int                         m_disp_pipe;

    int                         m_full_average;
    int                         m_curr_idx;
    uint64_t                    m_history[MAX_HISTORY];

    int                         m_measure_started;
    uint64_t                    m_measure_accum;
    uint32_t                    m_measure_nbsamp;
};

extern etrace_if etrace;

#endif

#endif
