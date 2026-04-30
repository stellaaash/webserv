#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "Request.hpp"
#include "cgi.hpp"

/* If no "Status: XXX" returned by cgi, set http response as 200 OK by default.
    TODOS :

    Fetch Interpreter and script path from Target location.
    Verify with stat the access to em files.

    Not handle DELETE request on Cgis ? Or send to CGIs but they don't accept it.
    Two simple CGIs that do something, one in Ruby, one in Python.
    Have them identify if it's GET/POST/DELETE and log info.

    Have CGIs return headers on stdout.
    Webserv must parse CGI headers to build html responses.

    Internal server error if the CGI script couldn't be exec (launch_cgi() fail). 500
    405 if Method is not allowed.

    Add CGI timeout.

    Webserv-sided :
    Detect if target is a CGI.
    Check script/interpreter integrity, missing, not allowed.
    Call launch_cgi with CgiRequest object.
*/

static std::string method_to_string(HttpMethod method) {
    if (method == GET) return "GET";
    if (method == POST) return "POST";
    if (method == DELETE) return "DELETE";
    return "";
}

/**
 * @brief Builds a mock CgiRequest struct to test cgi execution.
 */
CgiRequest build_mock_cgi_request(const Request& req) {
    CgiRequest cgi;

    cgi.interpreter = extract_interpreter(req);
    cgi.script_path = req.target().substr(0, req.target().find('?'));
    cgi.method = method_to_string(req.method());
    cgi.query_string = extract_query_string(req.target());

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
 * Returns a CgiResult struct containing the output string,
 * the error string if present, the status from the cgi and
 * exit code from the fork.
 */
CgiResult launch_cgi(const CgiRequest& req) {
    CgiPipes p;

    if (!init_pipes(p)) return make_error_result("pipe failed");

    pid_t pid = fork();

    if (pid == -1) {
        close_all_pipes(p);
        return make_error_result("fork failed");
    }

    if (pid == 0) {
        setup_child_pipes(p);

        std::vector<std::string> env = build_env(req);
        char**                   envp = build_c_array(env);
        char*                    argv[3];

        argv[0] = const_cast<char*>(req.interpreter.c_str());
        argv[1] = const_cast<char*>(req.script_path.c_str());
        argv[2] = NULL;

        execve(req.interpreter.c_str(), argv, envp);
        _exit(127);
    }

    setup_parent_pipes(p);

    if (req.body_is_file)
        write_file_to_fd(req.body_path, p.stdin_pipe[1]);
    else
        write_all(p.stdin_pipe[1], req.body);

    close_fd(p.stdin_pipe[1]);

    CgiResult result;
    result.output = read_all(p.stdout_pipe[0]);
    result.error = read_all(p.stderr_pipe[0]);

    close_fd(p.stdout_pipe[0]);
    close_fd(p.stderr_pipe[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    // TODO, Cgi returned status.
    result.status = 0;
    result.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    return result;
}
