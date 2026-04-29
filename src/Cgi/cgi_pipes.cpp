#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "cgi.hpp"

// Pipe utils for launch_cgi()

void setup_child_pipes(CgiPipes& p) {
    dup2(p.stdin_pipe[0], STDIN_FILENO);
    dup2(p.stdout_pipe[1], STDOUT_FILENO);
    dup2(p.stderr_pipe[1], STDERR_FILENO);

    close_all_pipes(p);
}

void setup_parent_pipes(CgiPipes& p) {
    close_fd(p.stdin_pipe[0]);
    close_fd(p.stdout_pipe[1]);
    close_fd(p.stderr_pipe[1]);
}

bool init_pipes(CgiPipes& p) {
    if (pipe(p.stdin_pipe) == -1) return false;

    if (pipe(p.stdout_pipe) == -1) {
        close_pipe(p.stdin_pipe);
        return false;
    }

    if (pipe(p.stderr_pipe) == -1) {
        close_pipe(p.stdin_pipe);
        close_pipe(p.stdout_pipe);
        return false;
    }

    return true;
}

/**
 * @brief xclose gem alarm
 */
void close_fd(int& fd) {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

void close_pipe(int pipefd[2]) {
    close_fd(pipefd[0]);
    close_fd(pipefd[1]);
}

void close_all_pipes(CgiPipes& p) {
    close_pipe(p.stdin_pipe);
    close_pipe(p.stdout_pipe);
    close_pipe(p.stderr_pipe);
}
