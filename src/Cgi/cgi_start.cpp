#include <fcntl.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "cgi.hpp"

// TODO Replace with socket_utils.cpp set_nonblocking()
static void set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static CgiProcess make_failed_process() {
    CgiProcess proc;

    proc.pid = -1;
    proc.stdout_fd = -1;
    proc.stderr_fd = -1;

    return proc;
}

static std::string method_to_string(HttpMethod method) {
    if (method == GET) return "GET";
    if (method == POST) return "POST";
    if (method == DELETE) return "DELETE";
    return "";
}

/**
 * @brief Builds a mock CgiRequest struct to test cgi execution.
 */
// TODO Will eventually be replaced with RePro logic
CgiRequest build_mock_cgi_request(const Request& req) {
    CgiRequest cgi;

    cgi.interpreter = extract_interpreter(req);  // /usr/bin/python3
    cgi.script_path = std::string("html").append(
        req.target().substr(0, req.target().find('?')));    // html/cgi-bin/test.py
    cgi.method = method_to_string(req.method());            // GET
    cgi.query_string = extract_query_string(req.target());  // hello=world

    if (req.has_header("Content-Type"))
        cgi.content_type = req.header("Content-Type")->second;
    else
        cgi.content_type = "";

    if (req.method() == POST) {
        cgi.content_length = req.content_length();

        if (req.is_body_spooled()) {
            cgi.body_is_file = true;
            cgi.body_path = req.body_path();
            cgi.body = "";
        } else {
            cgi.body_is_file = false;
            cgi.body = req.body();
            cgi.body_path = "";
        }
    } else {
        cgi.body_is_file = false;
        cgi.body = "";
        cgi.body_path = "";
        cgi.content_length = 0;
    }

    return cgi;
}

/**
 * @brief Executes cgi with a given CgiRequest struct.
 *
 * @return A CgiProcess struct containing the process id and fds.
 */
CgiProcess start_cgi(const CgiRequest& req) {
    CgiPipes pipes;

    // TODO Instead of returning a CgiProcess in a specific state to signify errors, throw an
    // exception
    if (!init_pipes(pipes)) return make_failed_process();

    pid_t pid = fork();

    if (pid == -1) {
        close_all_pipes(pipes);
        return make_failed_process();
    }

    if (pid == 0) {
        setup_child_pipes(pipes);

        std::vector<std::string> env = build_env(req);
        char**                   envp = build_c_array(env);

        char* argv[3];
        // FIXME If the script ever writes to these, we got undefined behavior
        // It should copy the data instead of const_casting it
        argv[0] = const_cast<char*>(req.interpreter.c_str());
        argv[1] = const_cast<char*>(req.script_path.c_str());
        argv[2] = NULL;

        execve(req.interpreter.c_str(), argv, envp);
        _exit(127);
    }

    setup_parent_pipes(pipes);

    set_non_blocking(pipes.stdout_pipe[0]);
    set_non_blocking(pipes.stderr_pipe[0]);

    if (req.body_is_file)
        write_file_to_fd(req.body_path, pipes.stdin_pipe[1]);
    else
        write_all(pipes.stdin_pipe[1], req.body);

    close_fd(pipes.stdin_pipe[1]);

    CgiProcess proc;
    proc.pid = pid;
    proc.stdout_fd = pipes.stdout_pipe[0];
    proc.stderr_fd = pipes.stderr_pipe[0];

    return proc;
}
