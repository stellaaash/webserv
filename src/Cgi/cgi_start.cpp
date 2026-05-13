#include <fcntl.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "cgi.hpp"
#include "file_manager.hpp"
#include "socket_utils.hpp"

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
 * @brief Builds a CgiRequest struct from a HTTP Request instance.
 * This CgiRequest will then be used to launch a CGI process.
 */
CgiRequest build_cgi_request(const std::string& relative_path, const Request& request) {
    CgiRequest cgi;

    cgi.interpreter = extract_interpreter(request);                            // /usr/bin/python3
    cgi.script_path = resolve_path(relative_path, request.config()->root);     // Full path
    cgi.script_name = request.target().substr(0, request.target().find('?'));  // /cgi-bin/test.py
    cgi.method = method_to_string(request.method());                           // GET
    cgi.query_string = request.query_string();                                 // hello=world

    if (request.has_header("content-type"))
        cgi.content_type = request.header("content-type")->second;
    else
        cgi.content_type = "";

    if (request.method() == POST) {
        cgi.content_length = request.content_length();

        if (request.is_body_spooled()) {
            cgi.body_is_file = true;
            cgi.body_path = request.body_path();
            cgi.body = "";
        } else {
            cgi.body_is_file = false;
            cgi.body = request.body();
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
        argv[0] = const_cast<char*>(req.interpreter.c_str());
        argv[1] = const_cast<char*>(req.script_path.c_str());
        argv[2] = NULL;

        execve(req.interpreter.c_str(), argv, envp);
        _exit(127);
    }

    setup_parent_pipes(pipes);

    set_nonblocking(pipes.stdout_pipe[0]);
    set_nonblocking(pipes.stderr_pipe[0]);

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
