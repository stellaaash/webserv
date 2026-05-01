#include "CgiHandler.hpp"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ConnectionHandler.hpp"
#include "ConnectionManager.hpp"
#include "Logger.hpp"
#include "cgi.hpp"

CgiHandler::CgiHandler(pid_t pid, int stdout_fd, int stderr_fd, long timeout,
                       ConnectionHandler* client)
    : _pid(pid),
      _stdout_fd(stdout_fd),
      _stderr_fd(stderr_fd),
      _timeout(timeout),
      _start(std::time(NULL)),
      _client(client),
      _stdout_eof(false),
      _stderr_eof(false) {}

CgiHandler::~CgiHandler() {
    if (_stdout_fd >= 0) {
        close(_stdout_fd);
        _stdout_fd = -1;
    }
    if (_stderr_fd >= 0) {
        close(_stderr_fd);
        _stderr_fd = -1;
    }
    Logger(LOG_DEBUG) << "[CGI] Handler destroyed for pid " << _pid;
}

int CgiHandler::fd() const {
    return _stdout_fd;
}

uint32_t CgiHandler::interests() const {
    return EPOLLIN | EPOLLHUP | EPOLLERR;
}

void CgiHandler::abort_cgi() {
    Logger(LOG_ERROR) << "[CGI] abort";

    if (_pid > 0) {
        kill(_pid, SIGKILL);
        waitpid(_pid, NULL, 0);
        _pid = -1;
    }
}

void CgiHandler::detach_client() {
    _client = NULL;
}

bool CgiHandler::is_timed_out() const {
    return (std::time(NULL) - _start) > _timeout;
}

void CgiHandler::timeout_connection() {
    abort_cgi();

    if (_client) {
        _client->clear_cgi_handler();
        _client->finish_cgi("", CGI_TIMEOUT, -1);
        _client = NULL;
    }
}

void CgiHandler::read_stdout() {
    char buffer[4096];

    while (true) {
        ssize_t n = read(_stdout_fd, buffer, sizeof(buffer));

        if (n > 0) {
            _output.append(buffer, static_cast<size_t>(n));
        } else if (n == 0) {
            _stdout_eof = true;
            break;
        } else {
            if (errno == EINTR) continue;
            break;
        }
    }
}

void CgiHandler::read_stderr() {
    if (_stderr_fd < 0) return;

    char buffer[4096];

    while (true) {
        ssize_t n = read(_stderr_fd, buffer, sizeof(buffer));

        if (n > 0) {
            _error.append(buffer, static_cast<size_t>(n));
        } else if (n == 0) {
            _stderr_eof = true;
            break;
        } else {
            if (errno == EINTR) continue;
            break;
        }
    }
}

ConnectionHandler* CgiHandler::client() const {
    return _client;
}

bool CgiHandler::handle_event(ConnectionManager& manager, uint32_t events) {
    (void)manager;

    // Read stdout on closed pipe or error.
    if (events & (EPOLLIN | EPOLLHUP | EPOLLERR)) read_stdout();

    // Read stderr if there's anything
    read_stderr();

    int status = 0;
    // is cgi done
    pid_t ret = waitpid(_pid, &status, WNOHANG);
    // pipe closed, stdout sent HUP, can be considered done
    if (ret == 0 && (events & EPOLLHUP)) {
        ret = waitpid(_pid, &status, 0);
    }

    // Cgi still running
    if (ret == 0) return true;

    // waitpid error
    if (ret == -1) {
        if (!_error.empty()) {
            Logger(LOG_ERROR) << "[CGI] stderr: " << _error;
        }
        if (_client) _client->finish_cgi(_output, CGI_ERROR, -1);
        return false;
    }

    // Cgi done
    int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    if (!_error.empty()) {
        Logger(LOG_ERROR) << "[CGI] stderr: " << _error;
    }

    if (_client) {
        _client->clear_cgi_handler();
        int cgi_status = (exit_code == 0 ? CGI_OK : CGI_ERROR);
        _client->finish_cgi(_output, cgi_status, exit_code);
        _client = NULL;
    }

    return false;
}
