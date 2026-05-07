#include "Connection.hpp"

#include <sys/socket.h>
#include <sys/types.h>

#include <cassert>
#include <cerrno>
#include <cstring>

#include "Logger.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "config.hpp"
#include "request_parser.hpp"

Connection::Connection(const ConfigServer* const config, int socket)
    : _config(config),
      _request(),
      _response(),
      _socket(socket),
      _read_buffer(),
      _read_index(0),
      _write_buffer(),
      _write_index(0),
      _pending_handler(NULL) {
    assert(config && "ConfigServer pointer");
    assert(socket > 2 && "Valid Socket Number");
}

Connection::~Connection() {
    if (_pending_handler) delete _pending_handler;
}

const Request& Connection::request() const {
    return _request;
}

const Response& Connection::response() const {
    return _response;
}

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
            // FIXME checking errno after a read/write is forbidden
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;  // nothing to read
            Logger(LOG_ERROR) << "[Connection::receive_data] recv: " << std::strerror(errno);
            return -1;
        }
    }

    return total;
}

bool Connection::handle_content_length_header(Request& request) {
    size_t content_length = 0;

    if (request.has_header("Content-Length") &&
        !parse_content_length_value(request.header("Content-Length")->second, content_length)) {
        request.set_error_status(400);
        request.set_status(REQ_ERROR);
        return false;
    }
    request.set_content_length(content_length);
    if (content_length > _config->client_max_body_size) {
        request.set_error_status(413);
        request.set_status(REQ_ERROR);
        return false;
    }
    if (content_length > 0) {
        request.set_status(REQ_HEADERS);
        return false;
    }
    return true;
}

/**
 * @brief This functions takes ownersihp of the _pending_handler CgiHandler pointer, and returns
 * it. It sets the internal pointer to NULL to change ownership to the caller of this function.
 */
CgiHandler* Connection::grab_pending_handler() {
    CgiHandler* handler = NULL;

    if (_pending_handler) {
        handler = _pending_handler;
        _pending_handler = NULL;
    }
    return handler;
}

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

void Connection::set_config(const ConfigServer* const config) {
    assert(config && "ConfigServer pointer");

    _config = config;
}

void Connection::set_request(const Request& request) {
    _request = request;
}

void Connection::set_response(const Response& response) {
    _response = response;
}

void Connection::queue_write(const std::string& data) {
    _write_buffer += data;
}

bool Connection::has_pending_write() const {
    return _write_index < _write_buffer.size();
}
