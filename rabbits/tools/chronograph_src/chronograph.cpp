#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

//#define DEBUG_INPUT

GdkPixmap           **pixmap = NULL, *endPartPixmap = NULL;
GtkWidget           **drawing_area = NULL;
GdkGC               *gc_grid = NULL;
GdkFont             *fntGrid = NULL;
GdkGC               **gc_grf = NULL;
int                 ngc_grf;
GdkGC               *gc_pixmap_cpy = NULL;
pthread_t           thread_io = (pthread_t) NULL;
int                 width_new_part = 10;
int                 ngrf, ngrfvisible, *ngrf_in_grf;
long long           *val_min_grf, *val_max_grf;
bool                bAvg = false;
unsigned long       cnt_values = 0;
unsigned long       ms_between_values;
unsigned char       *new_value, *last_value, nbytes_new_value;
unsigned long       *px_grid_text;

#define PX_GRID_BORDER    5

static void destroy (GtkWidget *widget, gpointer data)
{
    gtk_main_quit ();
}

static gboolean delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    return FALSE; //call destroy
}

static gboolean label_press_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    GtkWidget       *w = drawing_area[(int)data];
    if (GTK_WIDGET_VISIBLE (w))
    {
        if (ngrfvisible > 1)
        {
            gtk_widget_hide (w);
            ngrfvisible--;
        }
    }
    else
    {
        gtk_widget_show (w);
        ngrfvisible++;
    }

    return TRUE;
}

static gboolean label_enter_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    GdkColor        color;
    gdk_color_parse ("blue", &color);
    gtk_widget_modify_fg ((GtkWidget *)data, GTK_STATE_NORMAL, &color);
}

static gboolean label_leave_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    GdkColor        color;
    gdk_color_parse ("black", &color);
    gtk_widget_modify_fg ((GtkWidget *)data, GTK_STATE_NORMAL, &color);
}


int                 crt_tooltip_drawing_area = -1;
unsigned long       crt_tooltip_x = -1;

static gboolean drawing_area_leave_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    crt_tooltip_drawing_area = -1;
}

void UpdateDrawingAreaTooltip (bool redraw)
{
    char                    s[100];
    long long               tm;
    int                     idx = crt_tooltip_drawing_area;

    if (idx == -1)
        return;
    
    tm = cnt_values;
    tm -= (drawing_area[idx]->allocation.width - 1 - crt_tooltip_x) / width_new_part;
    if (tm < 0) tm = 0;
    tm *= ms_between_values;
    sprintf (s, "%lld ms", tm);
    gtk_widget_set_tooltip_text (drawing_area[idx], s);
    
    if (redraw)
        gtk_tooltip_trigger_tooltip_query (gtk_widget_get_display(drawing_area[idx]));
}

static gboolean drawing_area_motion (GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    crt_tooltip_drawing_area = (int) data;
    crt_tooltip_x = (unsigned long) event->x;
    UpdateDrawingAreaTooltip (false);
}

gboolean expose_event_callback (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    gdk_draw_drawable (widget->window, widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
            * (GdkPixmap **) data, 0, 0, 0, 0, 
            widget->allocation.width, widget->allocation.height);

    return TRUE;
}

GtkWidget *make_title_label (const char *text)
{
    GtkWidget *label;
    char *full;

    full = g_strdup_printf ("<span weight=\"bold\">%s</span>", text);
    label = gtk_label_new (full);
    g_free (full);

    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

    return label;
}

void InitGCs ()
{
    GdkColor            color;
    int                 i;
    
    gc_grid = gdk_gc_new (drawing_area[0]->window);
    gdk_color_parse ("#238928", &color);
    gdk_color_alloc (gdk_colormap_get_system (), &color);
    gdk_gc_set_foreground (gc_grid, &color);
    gdk_gc_set_line_attributes (gc_grid, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
    
    static const char *color_grf[] = {"#FFE700", "#00A2FF", "#00FF82", "#B600FF"};
    ngc_grf = sizeof (color_grf) / sizeof (char*);
    gc_grf = new GdkGC*[ngc_grf];
    for (i = 0; i < ngc_grf; i++)
    {
        gc_grf[i] = gdk_gc_new (drawing_area[0]->window);
        gdk_color_parse (color_grf[i], &color);
        gdk_color_alloc (gdk_colormap_get_system (), &color);
        gdk_gc_set_foreground (gc_grf[i], &color);
        gdk_gc_set_line_attributes (gc_grf[i], 2, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
    }
    
    fntGrid = gdk_font_load ("-*-helvetica-medium-r-*-*-14-*-*-*-*-*-*");
}

void InitPixmap ()
{
    int             width = drawing_area[0]->allocation.width;
    int             height = drawing_area[0]->allocation.height;
    char            stext[50];
    int             i, j, x, y, yh, yt, yth, idx_min_width, idx_max_width, max_width, font_height;
    
    px_grid_text = new unsigned long [ngrf];
    for (j = 0; j < ngrf; j++)
    {
        sprintf (stext, "%llu", val_max_grf[j]);
        px_grid_text[j] = gdk_string_width (fntGrid, stext) + 2 * PX_GRID_BORDER;
        if (!j || px_grid_text[j] < px_grid_text[idx_min_width])
            idx_min_width = j;
        if (!j || px_grid_text[j] > px_grid_text[idx_max_width])
            idx_max_width = j;
    }
    max_width = px_grid_text[idx_max_width];
    font_height = gdk_string_height (fntGrid, "197");

    for (j = 0; j < ngrf; j++)
    {
        px_grid_text[j] = max_width;

        gdk_draw_rectangle (pixmap[j], drawing_area[j]->style->black_gc, TRUE, 0, 0, width , height);
    
        y = PX_GRID_BORDER;
        yh = (height - PX_GRID_BORDER * 2) / 4;
        yt = PX_GRID_BORDER + font_height;
        yth = (height - PX_GRID_BORDER * 2 - 5 * font_height) / 4 + font_height;
        
        for (i = 0; i < 5; i++)
        {
            sprintf (stext, "%llu", val_min_grf[j] + (val_max_grf[j] - val_min_grf[j]) * (4 - i) / 4 );
            
            x = max_width - gdk_string_width (fntGrid, stext) - PX_GRID_BORDER;
            gdk_draw_text (pixmap[j], fntGrid, gc_grid, x, yt, stext, strlen (stext));
            yt += yth;
        
            gdk_draw_line (pixmap[j], gc_grid, px_grid_text[j], y, width - 1 - PX_GRID_BORDER, y);
            y += yh;
        }
        
        gdk_draw_line (pixmap[j], gc_grid, px_grid_text[j], 
            PX_GRID_BORDER, px_grid_text[j], height - 1 - PX_GRID_BORDER);
        gdk_draw_line (pixmap[j], gc_grid, width - 1 - PX_GRID_BORDER, 
            PX_GRID_BORDER, width - 1 - PX_GRID_BORDER, height - 1 - PX_GRID_BORDER);
    }

    gc_pixmap_cpy = gdk_gc_new (pixmap[idx_min_width]);
    endPartPixmap = gdk_pixmap_new (drawing_area[idx_min_width]->window, 
        width - px_grid_text[idx_min_width] - 2, height, -1);
    gdk_window_copy_area (endPartPixmap, gc_pixmap_cpy, 0, 0,
        pixmap[idx_min_width], px_grid_text[idx_min_width] + 1, 0, 
        width - px_grid_text[idx_min_width] - 2, height);
}

void update_graph ()
{
    int                 i, j, k, width, height;
    unsigned long       cval, clval, idx_new_value = 0, sval = 0, slval = 0;
    GtkWidget           *widget;
    GdkPixmap           *pmap;

    cnt_values++;

    for (i = 0; i < ngrf; i++)
    {
        widget = drawing_area[i];
        width = widget->allocation.width;
        height = widget->allocation.height;
        pmap = pixmap[i];

        gdk_window_copy_area (pmap, gc_pixmap_cpy, px_grid_text[i] + 1, 0, pmap,
            px_grid_text[i] + 1 + width_new_part,
            0,
            width - (px_grid_text[i] + 2 + width_new_part + PX_GRID_BORDER),
            height);

        gdk_window_copy_area (pmap, gc_pixmap_cpy, width - (PX_GRID_BORDER + 1 + width_new_part), 0, 
            endPartPixmap, 0, 0, width_new_part, height);

        if (i == ngrf - 1 && bAvg)
        {
            cval = sval / nbytes_new_value;
            clval = slval / nbytes_new_value;
            
            gdk_draw_line (pmap, gc_grf[0], width - (PX_GRID_BORDER + 2 + width_new_part),
                           PX_GRID_BORDER + 100 - clval, width - (PX_GRID_BORDER + 2), PX_GRID_BORDER + 100 - cval);

        }
        else
        {
            k = idx_new_value + ngrf_in_grf[i];
            for (j = idx_new_value; j < k; j++)
            {
                cval = new_value[j];
                sval += cval;
                clval = last_value[j];
                slval += clval;
                
                gdk_draw_line (pmap, gc_grf[(j - idx_new_value) % ngc_grf],
                    width - (PX_GRID_BORDER + 2 + width_new_part),
                    PX_GRID_BORDER + 100 - clval, width - (PX_GRID_BORDER + 2), PX_GRID_BORDER + 100 - cval);
                
                last_value[j] = new_value[j];
            }
            idx_new_value = k;
        }
        
        gdk_draw_drawable (widget->window, widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
            pmap, 0, 0, 0, 0, width, height);
    }
}

void *thread_io_fc (void *arg)
{
    while (1)
    {    
        #ifndef DEBUG_INPUT
        if (nbytes_new_value != ::read (0, new_value, nbytes_new_value))
        {
            printf ("Pipe closed!\n");
            gdk_threads_enter ();
            gtk_main_quit ();
            gdk_threads_leave ();
            break;
        }
        #else
        fscanf (stdin, "%lu %lu %lu %lu %lu", &new_value[0], &new_value[1], &new_value[2], &new_value[3], &new_value[4]);
        #endif

        gdk_threads_enter ();
        update_graph ();
        gdk_flush ();
        gdk_threads_leave ();
    }
    
    return NULL;
}

int main (int argc, char **argv)
{
    char            stitle[200], slabel[100];
    unsigned int    val;
    int                i;
    int             ret;
    
    #ifndef DEBUG_INPUT
    if ((4 != (ret = read (0, &val, 4))) || (val != read (0, stitle, val)))
    {
      fprintf (stderr, "Title cannot be read : ret = %d ... val %d!\n", ret, val);
      return 1;
    }
    
    if (4 != read (0, &val, 4))
    {
        fprintf (stderr, "Number of graphics cannot be read!\n");
        return 1;
    }
    ngrf = val & 0x7F;
    bAvg = (val & 0x80);
    
    if (4 != read (0, &ms_between_values, 4))
    {
        fprintf (stderr, "Number of ms between 2 calls cannot be read!\n");
        return 1;
    }
    
    ngrf_in_grf = new int[ngrf];
    val_min_grf = new long long[ngrf + 1];
    val_max_grf = new long long[ngrf + 1];
    nbytes_new_value = 0;
    for (i = 0; i < ngrf; i++)
    {
        if (4 != read (0, &ngrf_in_grf[i], 4) || 
                  8 != read (0, &val_min_grf[i], 8) ||
                  8 != read (0, &val_max_grf[i], 8)
           )
        {
            fprintf (stderr, "Info for graphic %d cannot be read!\n", i);
            return 1;
        }
        nbytes_new_value += ngrf_in_grf[i];
    }    
    #else
    ngrf = 4;
    bAvg = true;
    ms_between_values = 20;
    ngrf_in_grf = new int[ngrf];
    nbytes_new_value = 5;
    ngrf_in_grf[0] = 1; ngrf_in_grf[1] = 2; ngrf_in_grf[2] = 1; ngrf_in_grf[3] = 1;
    val_min_grf = new long long[ngrf + 1];
    val_min_grf[0] = 0; val_min_grf[1] = 10000; val_min_grf[2] = 120000; val_min_grf[3] = 1230000;
    val_max_grf = new long long[ngrf + 1];
    val_max_grf[0] = 100; val_max_grf[1] = 120000; val_max_grf[2] = 2120000; val_max_grf[3] = 111230000;
    strcpy (stitle, "Test");
    #endif

    val_min_grf[ngrf] = 0;
    val_max_grf[ngrf] = 0;
    for (i = 0; i < ngrf; i++)
    {
        val_min_grf[ngrf] += val_min_grf[i];
        val_max_grf[ngrf] += val_max_grf[i];
    }
    val_min_grf[ngrf] /= ngrf;
    val_max_grf[ngrf] /= ngrf;
    if (bAvg)
        ngrf++;
    ngrfvisible = ngrf;
    new_value = new unsigned char[nbytes_new_value];
    last_value = new unsigned char[nbytes_new_value];

    memset (last_value, 0, nbytes_new_value);

    /* init threads */    
    g_thread_init(NULL);
    gdk_threads_init();

    gtk_init (&argc, &argv);
    
    //create window
    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_resizable (GTK_WINDOW (window), true);
    gtk_window_set_title (GTK_WINDOW (window), stitle);
    g_signal_connect (G_OBJECT (window), "delete_event", G_CALLBACK (delete_event), NULL);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (destroy), NULL);
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    gtk_window_set_icon_name (GTK_WINDOW (window), "utilities-system-monitor");

    GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (window), scroll);
    gtk_widget_show (scroll);

    //create vbox
    GtkWidget        *vbox = gtk_vbox_new (FALSE, 4);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroll), vbox);
    gtk_widget_show (vbox);

    drawing_area = new GtkWidget * [ngrf];
    pixmap = new GdkPixmap * [ngrf];

    GtkWidget   **w4size = new GtkWidget* [ngrf];
    
    for (i = 0; i < ngrf; i++)
    {
        //create hbox
        GtkWidget        *box = gtk_hbox_new (FALSE, 10);
        gtk_container_set_border_width (GTK_CONTAINER (box), 2);
        gtk_container_add (GTK_CONTAINER (vbox), box);
        gtk_widget_show (box);

        #ifndef DEBUG_INPUT
        if ((4 != read (0, &val, 4)) || (val != read (0, slabel, val)))
        {
            fprintf (stderr, "Label %d cannot be read!\n", i);
            return 1;
        }
        #else
        if (i == ngrf - 1 && bAvg)
            strcpy (slabel, "All CPUs (%)");
        else
            sprintf (slabel, "CPU %4d (%%)", i);
        #endif

        GtkWidget        *event_box = gtk_event_box_new ();
        gtk_box_pack_start (GTK_BOX (box), event_box, FALSE, FALSE, 3);
        gtk_widget_show (event_box);
        GtkWidget        *name_label = make_title_label (slabel);
        w4size[i] = name_label;
        gtk_widget_show (name_label);
        gtk_container_add (GTK_CONTAINER (event_box), name_label);
        g_signal_connect (G_OBJECT (event_box), "button_press_event", G_CALLBACK (label_press_event), (void *) i);
        g_signal_connect (G_OBJECT (event_box), "enter_notify_event", G_CALLBACK (label_enter_event), name_label);
        g_signal_connect (G_OBJECT (event_box), "leave_notify_event", G_CALLBACK (label_leave_event), name_label);
        gtk_widget_realize (event_box);
        gdk_window_set_cursor (event_box->window, gdk_cursor_new (GDK_HAND2));
        
        //drawing area
        drawing_area[i] = gtk_drawing_area_new ();
        gtk_widget_set_size_request (drawing_area[i], 540, 111);
        gtk_widget_set_has_tooltip (drawing_area[i], TRUE);
        g_signal_connect (G_OBJECT (drawing_area[i]), "expose_event", G_CALLBACK (expose_event_callback), &pixmap[i]);
        g_signal_connect (G_OBJECT (drawing_area[i]), "leave_notify_event", G_CALLBACK (drawing_area_leave_event), NULL);
        g_signal_connect (G_OBJECT (drawing_area[i]), "motion_notify_event", G_CALLBACK (drawing_area_motion),
                          (void *) i);
        gtk_widget_show (drawing_area[i]);
        gtk_box_pack_start (GTK_BOX (box), drawing_area[i], FALSE, FALSE, 3);
    }

    gtk_widget_show (window);
    
    for (i = 0; i < ngrf; i++)
    {
        pixmap[i] = gdk_pixmap_new (drawing_area[i]->window, 
            drawing_area[i]->allocation.width, drawing_area[i]->allocation.height, -1);
    }

    InitGCs ();
    InitPixmap ();

    //make the same width for the labels of all rows
    {
        int                 maxwidth = 0;
        GtkRequisition      crtwsize;
        for (i = 0; i < ngrf; i++)
        {
            gtk_widget_size_request (w4size[i], &crtwsize);
            if (crtwsize.width > maxwidth)
                maxwidth = crtwsize.width;
        }
        for (i = 0; i < ngrf; i++)
        {
            gtk_widget_size_request (w4size[i], &crtwsize);
            gtk_widget_set_size_request (w4size[i], maxwidth, crtwsize.height);
        }
        delete [] w4size;
        
        gtk_window_resize (GTK_WINDOW (window), maxwidth + 610,
            (ngrf <= 4) ? (ngrf * 119 + 39) : 650);
    }

    pthread_create (&thread_io, NULL, thread_io_fc, NULL);
    
    gdk_threads_enter ();
    gtk_main ();
    gdk_threads_leave ();

    return 0;    
}

