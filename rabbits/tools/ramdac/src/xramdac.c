/*
 *  Copyright (c) 2010 TIMA Laboratory
 *
 *  This file is part of Rabbits.
 *
 *  Rabbits is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Rabbits is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Rabbits.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h> 
#include "pthread.h"

#ifdef DEBUG_RAMDAC
#define DPRINTF(fmt, args...)                               \
    do { fprintf(stdout, "xramdac: " fmt , ##args); } while (0)
#else
#define DPRINTF(fmt, args...) do {} while(0)
#endif

#define EPRINTF(fmt, args...)                               \
    do { fprintf(stderr, "xramdac: " fmt , ##args); } while (0)


gboolean display_drawing_area (GtkWidget *widget,
                               GdkEventExpose *event,
                               gpointer user_data);
void display();

static GtkWidget			*darea;
static GtkWidget			*pWindow;
static unsigned char		*image[2];
static unsigned int			width;
static unsigned int			height;
static unsigned int		    components;
static unsigned int			wi;
static unsigned int			shmid[2];
static unsigned char		*copy;

static unsigned long        dummy;

void
clean_shm(void){
    int i = 0;
    DPRINTF("XRAMDAC exited\n");
    for(i = 0; i < 2; i++){
        shmdt(image[i]);
    }
}

int
main (int argc, char *argv[])
{
    unsigned int			key[2];
    char					xname[80] = "*** The screen ***";
    unsigned int			i;


    DPRINTF("xramdac launched\n");
    atexit(clean_shm);

    if( gtk_init_check (&argc, &argv) != TRUE ){
        EPRINTF("XRAMDAC gtk cannot be initialized\n");
        exit(EXIT_FAILURE);
    };

    DPRINTF("reading config\n");

    dummy = read(0, key, sizeof(key));
    dummy = read(0, &width, sizeof(unsigned int));
    dummy = read(0, &height, sizeof(unsigned int));
    dummy = read(0, &components, sizeof(unsigned int));
    dummy = read(0, xname, sizeof(xname));

    for( i = 0 ; i < 2 ; i++ )
    {
        DPRINTF("getting shm %x\n", key[i]);
        DPRINTF("%d, %d\n", width, height);
        DPRINTF("size: %d\n", components * width * height * sizeof(char));
        if ((shmid[i] = shmget(key[i], components * width * height * sizeof(char), 0400)) == -1)
        {
            perror("ERROR: Ramdac.shmget");
            exit(1);
        }
        if ( (image[i] = (unsigned char*)shmat(shmid[i], 0, 00400)) == (void *)-1) 
        {
            perror("ERROR: Ramdac.shmat");
            exit(1);
        }
    }

    copy = (unsigned char *) malloc (components * width * height * sizeof(char));
    memset(copy, 0x00, components * width * height * sizeof(char));

    pWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    if(components == 1)
        gtk_window_set_title (GTK_WINDOW (pWindow), "RAMDAC(gray)");
    if(components == 3)
        gtk_window_set_title (GTK_WINDOW (pWindow), "RAMDAC(RGB)");

    darea = gtk_drawing_area_new ();
    gtk_widget_set_size_request (darea, width, height);
    gtk_container_add (GTK_CONTAINER (pWindow), darea);
    gtk_signal_connect (GTK_OBJECT (darea), "expose-event",
                        GTK_SIGNAL_FUNC (display_drawing_area), NULL);
    g_signal_connect(G_OBJECT(pWindow), "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all (pWindow);

    wi = 0;

    signal(SIGUSR1, display);

    gtk_main ();
    return 0;
}

void display()
{
    DPRINTF("Xramdac: displaying area\n");
    memcpy(copy,image[wi], components * width * height * sizeof(char));
    display_drawing_area(darea, NULL, NULL);
    wi = (wi + 1) % 2;
    DPRINTF("Xramdac: end of display\n");
    return;
}

gboolean display_drawing_area (GtkWidget *widget,
                               GdkEventExpose *event,
                               gpointer user_data)
{
    if( components == 1)
    {
        gdk_draw_gray_image (widget->window, widget->style->fg_gc[GTK_STATE_NORMAL],
                             0, 0, width, height,
                             GDK_RGB_DITHER_MAX, copy, width);
    }
    else
    {
        if( components == 3)
        {
            gdk_draw_rgb_image (widget->window, widget->style->fg_gc[GTK_STATE_NORMAL],
                                0, 0, width, height,
                                GDK_RGB_DITHER_MAX, copy, width * 3);
        }
    }
    return TRUE;
}

/*
 * Vim standard variables
 * vim:set ts=4 expandtab tw=80 cindent syntax=c:
 *
 * Emacs standard variables
 * Local Variables:
 * mode: c
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
