#include "Connection.hpp"

#include <sys/socket.h>
#include <sys/types.h>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "Logger.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "config.hpp"
#include "file_manager.hpp"

Connection::Connection(const ConfigServer* const config, int socket)
    : _config(config),
      _request(),
      _response(),
      _socket(socket),
      _read_buffer(),
      _read_index(0),
      _write_buffer(),
      _write_index(0) {
    assert(config && "ConfigServer pointer");
    assert(socket > 2 && "Valid Socket Number");
    _request.set_client_max_body_size(_config->client_max_body_size);
}

Connection::~Connection() {}

const Request& Connection::request() const {
    return _request;
}

const Response& Connection::response() const {
    return _response;
}

/**
 * @brief Sends data through the connection socket. Each call of this functions sends a chunk of
 * size determined by the SEND_SIZE macro.
 */
ssize_t Connection::send_data() {
    if (_write_index >= _write_buffer.size()) return 0;

    // remaining bytes to send
    size_t remaining = _write_buffer.size() - _write_index;
    // max send size or remaining if smaller
    size_t chunk = remaining > SEND_SIZE ? SEND_SIZE : remaining;

    ssize_t n = send(_socket, _write_buffer.data() + _write_index, chunk, 0);
    if (n == 0)
        return 0;
    else if (n < 0) {
        Logger(LOG_ERROR) << "[Connection::send_data] send: " << strerror(errno);
        return -1;
    }
    // update write index with real bytes sent (can be lower than chunk)
    _write_index += static_cast<size_t>(n);

    // cleanup
    if (_write_index >= _write_buffer.size()) {
        _write_buffer.clear();
        _write_index = 0;
    }
    return n;
}

ssize_t Connection::receive_data() {
    char    buffer[RECV_SIZE];
    ssize_t total = 0;

    while (true) {
        ssize_t n = recv(_socket, buffer, sizeof(buffer), 0);
        if (n > 0) {
            _read_buffer.append(buffer, static_cast<size_t>(n));
            total += n;
        } else if (n == 0) {
            // closing client
            Logger(LOG_GENERAL) << "[CONN " << _socket << "] client closed recv0";
            return 0;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;  // nothing to read
            Logger(LOG_ERROR) << "[Connection::receive_data] recv: " << std::strerror(errno);
            return -1;
        }
    }
    // if (total > 0)
    //     std::cout << "[CONN " << _socket << "] received " << total << " bytes" << std::endl;

    return total;
}

/**
 * @brief This function either erases the read buffer of the connection if all its contents were
 * sent, or shrinks it after consuming either 8kb or half of it.
 */
void Connection::shrink_read_buffer() {
    if (_read_index == 0) return;

    if (_read_index >= _read_buffer.size()) {
        _read_buffer.clear();
        _read_index = 0;
        return;
    }
    // Clean buffer after consuming 8Kb or half the read buffer.
    if (_read_index >= 8192 || _read_index * 2 >= _read_buffer.size()) {
        _read_buffer.erase(0, _read_index);
        _read_index = 0;
    }
}

ParsingStatus Connection::parse_request() {
    ParsingStatus status = parse(*_config, _read_buffer, _read_index, _request);
    shrink_read_buffer();
    return status;
}

/**
 * @brief Processes a Request, returning an appropriate Response object.
 *
 * @description The Response object will either contain a valid file descriptor to read from, or a
 * filled body string containing the generated content.
 */
// TODO Move processing related functions to a dedicated file
// Technically, Connection::process_request could be in a separate file as well
// TODO: Return status of the processing
void Connection::process_request() {
    const ConfigLocation* const config = _request.config();
    FilePath                    resource_path;
    FilePath                    relative_path;

    assert(config && "Config pointer valid");

    _response.set_version(1, 1);

    // The relative path is the path from the root of the location to the resource
    relative_path = _request.target().substr(config->name.length(), _request.target().npos);
    Logger(LOG_DEBUG) << "[!] - Relative path: " << relative_path;

    // Isolate the resource needed
    if (relative_path.empty() && !config->index.empty()) {
        resource_path = config->index;
    } else {
        resource_path = config->root + "/" + relative_path;
    }

    if (_request.config()->allowed_methods.find(_request.method()) ==
        _request.config()->allowed_methods.end()) {
        _response.set_code(405);
        _response.set_response_string("Method Not Allowed");
        return;
    }

    Logger(LOG_DEBUG) << "[REPRO] - Fetching resource: " << resource_path;

    // Fetch the resource or generate content
    if (is_directory(resource_path) == true) {
        if (config->autoindex == true) {
            _response.append_body(create_listing(config->root));
            _response.set_code(200);
            _response.set_response_string("OK");
        } else
            _response.set_code(404);
    } else if (_request.method() == GET) {  // TODO One function per method
        int fd = fetch_file(resource_path);
        if (fd < 0) {
            perror("[process_request] - fetch_file");
            if (errno == EACCES)
                _response.set_code(403);
            else if (errno == ENOENT)
                _response.set_code(404);
            else if (errno == EINVAL || errno == ENAMETOOLONG)
                _response.set_code(400);
            else
                _response.set_code(500);
            return;
        }
        // TODO Throw or return error page (we could technically throw a Response)

        _response.set_fd(fd);
        _response.set_code(200);
        _response.set_response_string("OK");
        // TODO Set header, MIME types??
    } else if (_request.method() == POST) {
        // TODO Store file or launch CGI
        _response.set_code(501);
        _response.set_response_string("Not Implemented");
    } else if (_request.method() == DELETE) {
        // TODO Remove file
        _response.set_code(501);
        _response.set_response_string("Not Implemented");
    } else {
        _response.set_code(501);
        _response.set_response_string("Not Implemented");
    }

    std::stringstream convert;
    if (_response.body().empty() == false)
        convert << _response.body().length();
    else
        convert << file_length(resource_path);
    // FIXME This line (and potentially others) are being called at every call of process_request,
    // and shouldn't be (this one in particular results in multiple Content-Length headers being
    // added to the request)
    _response.set_header("Content-Length", convert.str());

    // TODO Fetch the error page if needed and configured
}

void Connection::queue_write(const std::string& data) {
    _write_buffer += data;
}

bool Connection::has_pending_write() const {
    return _write_index < _write_buffer.size();
}

void Connection::set_config(const ConfigServer* const config) {
    assert(config && "ConfigServer pointer");

    _config = config;
}
