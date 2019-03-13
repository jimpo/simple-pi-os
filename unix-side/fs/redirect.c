#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <support/demand.h>

#include "redirect.h"

// redir:
//  fork/exec <pi_process>
// return two file descripts:
//  rd_fd = the file descriptor the parent reads from to see the child's
//      stdout/stderr output.
//  wr_fd = the file descriptor the parent writes to in order to write data
//      to the child's stdin.
//
// you will override the child's stdin/stdout/stderr using pipes and dup2.
//      recall: pipe[0] = read in, pipe[1] = write end.
int redir(int *rd_fd, int *wr_fd, char * const pi_process) {
    int parent_fds[2], child_fds[2];
    if (pipe(parent_fds) < 0) {
        sys_die(pipe, failed to create a pipe);
    }
    if (pipe(child_fds) < 0) {
        sys_die(pipe, failed to create a pipe);
    }

    pid_t pid = fork();
    if (pid == 0) {
        if (dup2(child_fds[0], STDIN_FILENO) < 0) {
            sys_die(dup2, failed to redirect stdin);
        }
        if (dup2(parent_fds[1], STDOUT_FILENO) < 0) {
            sys_die(dup2, failed to redirect stdout);
        }
        if (dup2(parent_fds[1], STDERR_FILENO) < 0) {
            sys_die(dup2, failed to redirect stderr);
        }

        if (close(parent_fds[0]) < 0) {
            sys_die(close, failed to close parent read pipe);
        }
        if (close(child_fds[1]) < 0) {
            sys_die(close, failed to close child write pipe);
        }

        execlp(pi_process, pi_process, NULL);
        sys_die(execlp, failed to exec pi_process);
    }

    *rd_fd = parent_fds[0];
    *wr_fd = child_fds[1];

    if (close(parent_fds[1]) < 0) {
        sys_die(close, failed to close parent write pipe);
    }
    if (close(child_fds[0]) < 0) {
        sys_die(close, failed to close child read pipe);
    }

    return 0;
}

int fd_putc(int fd, char c) {
    if(write(fd, &c, 1) != 1)
        sys_die(write, write failed);
    return 0;
}
void fd_puts(int fd, const char *msg) {
    while(*msg)
        fd_putc(fd, *msg++);
}


