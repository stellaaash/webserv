#ifndef CGI_HPP
#define CGI_HPP

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "Request.hpp"

typedef struct CgiPipes {
    int stdin_pipe[2];
    int stdout_pipe[2];
    int stderr_pipe[2];
} CgiPipes;

typedef struct CgiResult {
    int         status;
    int         exit_code;
    std::string output;
    std::string error;
} CgiResult;

typedef struct CgiRequest {
    std::string                        interpreter;
    std::string                        script_path;
    std::string                        method;
    std::string                        query_string;
    std::string                        content_type;
    std::string                        body;
    bool                               body_is_file;
    std::string                        body_path;
    size_t                             content_length;
    std::map<std::string, std::string> headers;
} CgiRequest;

// /Cgi/cgi_pipes.cpp
bool init_pipes(CgiPipes& p);
void setup_child_pipes(CgiPipes& p);
void setup_parent_pipes(CgiPipes& p);

// /Cgi/cgi_utils.cpp
void        close_fd(int& fd);
void        close_pipe(int pipefd[2]);
void        close_all_pipes(CgiPipes& p);
std::string extract_query_string(const std::string& target);
bool        write_file_to_fd(const std::string& path, int out_fd);

std::string              read_all(int fd);
bool                     write_all(int fd, const std::string& data);
std::string              to_string_size(size_t n);
std::vector<std::string> build_env(const CgiRequest& req);
char**                   build_c_array(std::vector<std::string>& values);
CgiResult                make_error_result(const std::string& message);

CgiRequest build_mock_cgi_request(const Request& req);
CgiResult  launch_cgi(const CgiRequest& req);

#endif
