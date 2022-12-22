#include <stdio.h>

main (ac, av)
    int ac;
    char **av;
{
    char *zipfile;
    int fd[2];

    switch (ac) {

    case 2:
        zipfile = av[1];
        break;

    case 1:
        zipfile = NULL;
        break;

    default:
        fputs (stderr, "usage: gziptocomp [zipfile]");
        exit (1);
    }

    if (pipe (fd) < 0) {
        perror ("pipe");
        exit (1);
    }

    switch (fork ()) {

    default:            /* the father */
        if (dup2 (fd[0], 0) < 0) {
            perror ("parent: dup2");
            exit (1);
        }
        close (fd[1]);
        execlp ("/bin/compress", "compress", NULL);
        perror ("execlp: compress");
        exit (1);

    case 0:             /* the son */
        if (dup2 (fd[1], 1) < 0) {
            perror ("child: dup2");
            exit (1);
        }
        close (fd[0]);
        execlp ("/bin/gzip", "gzip", "-cd", zipfile, NULL);
        perror ("execlp: unzip");
        exit (1);

    case -1:            /* Murphy's ghost */
        perror ("fork");
        exit (1);
    }
}
