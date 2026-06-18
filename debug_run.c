#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>

#define _POSIX_C_SOURCE 200809L

#include <time.h>
#include <unistd.h>
#include <unistd.h>
#include <sys/time.h>


/* For nanosleep/usleep declarations */
#include <unistd.h>







/*
 * C replacement for the old bash debug_run.sh.
 * Behavior:
 *  - run ./timepod
 *  - wait ~1s
 *  - send 'q' to its stdin
 *  - wait for exit
 *  - report whether timepod_days.bin exists
 */

static int file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}


int main(void) {
    const char *exe = "./timepod";
    const char *days_file = "./timepod_days.bin";


    int stdin_pipe[2];
    if (pipe(stdin_pipe) != 0) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* child */
        (void)close(stdin_pipe[1]);
        if (dup2(stdin_pipe[0], STDIN_FILENO) < 0) {

            perror("dup2");
            _exit(127);
        }
        (void)close(stdin_pipe[0]);

        /* exec */
        char *const argv[] = {"./timepod", NULL};
        execv(exe, argv);
        perror("execv");
        _exit(127);
    }

    /* parent */
    (void)close(stdin_pipe[0]);

    /* give the app a moment to render */
    {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200000; /* 200ms */
        select(0, NULL, NULL, NULL, &tv);
    }

    /* Send '1' to start a domain timer, wait a bit, then quit.
     * This should trigger tick/progress long enough to persist at least one completion
     * if the app is already implemented to mark on completion only.
     * (We still quit quickly; persistence may only happen if duration is short.)
     */
    {
        const char key = '1';
        (void)write(stdin_pipe[1], &key, 1);
    }

    /* wait a moment while running */
    {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 300000; /* 300ms */
        select(0, NULL, NULL, NULL, &tv);
    }

    /* now quit */
    {
        const char q = 'q';
        (void)write(stdin_pipe[1], &q, 1);
    }

    /* close stdin to provide EOF */
    (void)close(stdin_pipe[1]);




    int status = 0;
    (void)waitpid(pid, &status, 0);

    printf("debug_run: exited with status %d\n", status);
    if (file_exists(days_file)) {
        printf("debug_run: found %s\n", days_file);
    } else {
        printf("debug_run: did NOT find %s\n", days_file);
    }

    return 0;
}

