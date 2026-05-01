#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include "cgi.hpp"

/**
 * @brief Writes to an fd in case a body is a file (large size bodies)
 */
bool write_file_to_fd(const std::string& path, int out_fd) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) return false;

    char buffer[4096];

    while (true) {
        ssize_t bytes = read(fd, buffer, sizeof(buffer));

        if (bytes > 0) {
            if (!write_all(out_fd, std::string(buffer, static_cast<size_t>(bytes)))) {
                close(fd);
                return false;
            }
        } else if (bytes == 0) {
            break;
        } else {
            close(fd);
            return false;
        }
    }

    close(fd);
    return true;
}

/**
 * @brief Extract the interpreter from target.
 * Removes query string, checks for file extension, finds interpreter in config from extension
 * /cgi-bin/test.py?hello=world becomes "py", function returns FilePath stored in config.
 */
FilePath extract_interpreter(const Request& req) {
    const ConfigLocation* cfg = req.config();
    if (!cfg) return FilePath();

    std::string target = req.target();
    std::string path = target.substr(0, target.find('?'));

    std::string::size_type dot = path.rfind('.');
    if (dot == std::string::npos) return FilePath();

    std::string extension = path.substr(dot + 1);

    CgiIterator it = cfg->cgi.find(extension);
    if (it == cfg->cgi.end()) return FilePath();

    return it->second;
}

/**
 * @brief Minimal implementation to fetch the query string from the target.
 * /over/there?name=ferret
 */
std::string extract_query_string(const std::string& target) {
    size_t pos = target.find('?');

    if (pos == std::string::npos) return "";

    return target.substr(pos + 1);
}

std::string to_string_size(size_t n) {
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

static std::string make_env_entry(const std::string& key, const std::string& value) {
    return key + "=" + value;
}

std::vector<std::string> build_env(const CgiRequest& req) {
    std::vector<std::string> env;

    env.push_back(make_env_entry("GATEWAY_INTERFACE", "CGI/1.1"));
    env.push_back(make_env_entry("SERVER_PROTOCOL", "HTTP/1.1"));
    env.push_back(make_env_entry("REQUEST_METHOD", req.method));
    env.push_back(make_env_entry("SCRIPT_FILENAME", req.script_path));
    env.push_back(make_env_entry("SCRIPT_NAME", req.script_path));
    env.push_back(make_env_entry("QUERY_STRING", req.query_string));
    env.push_back(make_env_entry("CONTENT_LENGTH", to_string_size(req.content_length)));
    env.push_back(make_env_entry("CONTENT_TYPE", req.content_type));
    // We don't do PHP i guess, leaving it there for now as mystical knowledge cause php requires
    // this or cgi doesn't work for it apparently.
    if (req.interpreter.find("php") != std::string::npos)
        env.push_back(make_env_entry("REDIRECT_STATUS", "200"));

    return env;
}

/**
 * @brief Builds a C-like array with the Environment vars to feed execve()
 */
char** build_c_array(std::vector<std::string>& values) {
    char** array = new char*[values.size() + 1];

    for (size_t i = 0; i < values.size(); ++i) array[i] = const_cast<char*>(values[i].c_str());

    array[values.size()] = NULL;
    return array;
}

std::string read_all(int fd) {
    std::string result;
    char        buffer[4096];

    while (true) {
        ssize_t bytes = read(fd, buffer, sizeof(buffer));

        if (bytes > 0) {
            result.append(buffer, static_cast<size_t>(bytes));
        } else if (bytes == 0) {
            break;
        } else {
            break;
        }
    }

    return result;
}

bool write_all(int fd, const std::string& data) {
    size_t total = 0;

    while (total < data.size()) {
        ssize_t bytes = write(fd, data.c_str() + total, data.size() - total);

        if (bytes > 0)
            total += static_cast<size_t>(bytes);
        else
            return false;
    }

    return true;
}
