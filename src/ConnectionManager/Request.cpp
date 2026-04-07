#include "Request.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#include "HttpMessage.hpp"
#include "config.hpp"

#ifndef REQUEST_BODY_SPOOL_THRESHOLD
#define REQUEST_BODY_SPOOL_THRESHOLD (64 * 1024)
#endif

Request::Request()
    : _config(NULL),
      _target(""),
      _content_length(0),
      _client_max_body_size(0),
      _body_received(0),
      _method(UNDEFINED),
      _status(EMPTY),
      _error_status(0),
      _spool_threshold(REQUEST_BODY_SPOOL_THRESHOLD),
      _is_body_spooled(false),
      _body_fd(-1),
      _body_path("") {}

Request::Request(const ConfigLocation* const config, HttpMethod method)
    : _config(config),
      _target(""),
      _content_length(0),
      _client_max_body_size(0),
      _body_received(0),
      _method(method),
      _status(EMPTY),
      _error_status(0),
      _spool_threshold(REQUEST_BODY_SPOOL_THRESHOLD),
      _is_body_spooled(false),
      _body_fd(-1),
      _body_path("") {
    assert(config && "Config_Location pointer");
}

Request::~Request() {
    cleanup_temp_file();
}

HttpMethod Request::method() const {
    return _method;
}

ParsingStatus Request::status() const {
    return _status;
}

const std::string& Request::target() const {
    return _target;
}

size_t Request::content_length() const {
    return _content_length;
}

size_t Request::client_max_body_size() const {
    return _client_max_body_size;
}

void Request::set_client_max_body_size(size_t client_max_body_size) {
    _client_max_body_size = client_max_body_size;
}

size_t Request::body_received() const {
    return _body_received;
}

HttpCode Request::error_status() const {
    return _error_status;
}

bool Request::is_body_spooled() const {
    return _is_body_spooled;
}

const std::string& Request::body_path() const {
    return _body_path;
}

void Request::set_config(const ConfigLocation* const config) {
    assert(config && "Config_Server pointer");

    _config = config;
}

void Request::set_method(HttpMethod method) {
    _method = method;
}

void Request::set_status(ParsingStatus status) {
    if (status == ERROR) {
        _status = status;
        return;
    }
    assert(status >= _status && "Walking back status");
    _status = status;
}

void Request::set_target(const std::string& target) {
    _target = target;
}

void Request::set_content_length(size_t content_length) {
    _content_length = content_length;
}

void Request::set_error_status(HttpCode code) {
    _error_status = code;
}

bool Request::write_all(int fd, const char* data, size_t len) {
    size_t written = 0;

    while (written < len) {
        ssize_t n = write(fd, data + written, len - written);
        if (n < 0) {
            perror("[Request] write");
            return false;
        }
        written += static_cast<size_t>(n);
    }
    return true;
}

bool Request::open_temp_body_file() {
    if (_body_fd >= 0) return true;

    struct stat st;
    if (stat("tmp", &st) == -1 || !S_ISDIR(st.st_mode)) {
        std::cerr << "[Request::open_temp_body_file] tmp directory missing\n";
        return false;
    }

    static unsigned long counter = 0;

    for (int attempt = 0; attempt < 128; ++attempt) {
        std::ostringstream oss;
        oss << "tmp/webserv_body_" << reinterpret_cast<unsigned long>(this) << "_" << counter++;

        std::string path = oss.str();

        int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
        if (fd >= 0) {
            _body_fd = fd;
            _body_path = path;
            return true;
        }

        if (errno != EEXIST) {
            std::cerr << "[Request::open_temp_body_file] open: " << std::strerror(errno)
                      << std::endl;
            return false;
        }
    }

    std::cerr << "[Request::open_temp_body_file] failed to create unique temp file\n";
    return false;
}

/**
 * @brief Body becomes too big, transfer body to file
 */
bool Request::flush_memory_body_to_file() {
    if (_is_body_spooled) return true;
    if (!open_temp_body_file()) return false;

    const std::string& in_memory = body();
    if (!in_memory.empty() && !write_all(_body_fd, in_memory.data(), in_memory.size()))
        return false;

    _is_body_spooled = true;
    return true;
}

void Request::cleanup_temp_file() {
    if (_body_fd >= 0) {
        close(_body_fd);
        remove(_body_path.c_str());
        _body_fd = -1;
    }
}

bool Request::append_body_chunk(const char* data, size_t len) {
    if (len == 0) return true;

    if (!_is_body_spooled && body().size() + len > _spool_threshold) {
        if (!flush_memory_body_to_file()) return false;
    }

    if (_is_body_spooled) {
        if (!write_all(_body_fd, data, len)) return false;
    } else {
        append_body(std::string(data, len));
    }

    _body_received += len;
    return true;
}
