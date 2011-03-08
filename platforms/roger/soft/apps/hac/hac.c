#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "rabbitsha_ioctl.h"

char *program_name = NULL;
char *driver_file_name = NULL;
char *command = NULL;
char *host_file = NULL;

int syntax_help (void)
{
    printf ("Syntax: %s <driver_file> <command>\n", program_name);
    printf ("\tcommands: open <host_file>\n", program_name);
    printf ("\t          close\n", program_name);

    return 1;
}

int main (int argc, char **argv)
{
    int ret;

    program_name = argv[0];

    if (argc < 3)
        return syntax_help ();

    driver_file_name = argv[1];
    command = argv[2];

    int     fd = open (driver_file_name, O_RDWR);
    if (fd < 0)
    {
        printf ("%s: cannot open the file %s\n", program_name, driver_file_name);
        return 2;
    }

    if (!strcmp (command, "open"))
    {
        if (argc < 4)
        {
            printf ("%s: host file not specified in open command\n", program_name);
            close (fd);
            return syntax_help ();
        }
        host_file = argv[3];

        ret = ioctl (fd, RABBITSHA_IOCSOPEN, host_file);
        if (ret < 0)
        {
            printf ("%s: cannot open host file %s\n", program_name, host_file);
            close (fd);
            return 3;
        }
    }
    else
    if (!strcmp (command, "close"))
    {
        printf ("%s: host file close not implemented\n", program_name);
    }
    else
    {
        printf ("%s: invalid command %s\n", program_name, command);
        close (fd);
        return syntax_help ();
    }

    close (fd);

    return 0;
}


