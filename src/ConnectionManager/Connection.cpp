#include "Connection.hpp"

#include <sys/socket.h>
#include <sys/types.h>

#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>

#include "Request.hpp"
#include "Response.hpp"
#include "config.hpp"

Connection::Connection(const Config_Server* const config, int socket)
    : _config(config),
      _request(),
      _response(),
      _socket(socket),
      _read_buffer(),
      _read_index(0),
      _write_buffer(),
      _write_index(0) {
    assert(config && "Config_Server pointer");
    assert(socket > 2 && "Valid Socket Number");
}

Connection::Connection(const Connection& other)
    : _config(other._config),
      _request(other._request),
      _response(other._response),
      _socket(other._socket),
      _read_buffer(),
      _read_index(other._read_index),
      _write_buffer(),
      _write_index(other._write_index) {}

const Connection& Connection::operator=(const Connection& other) {
    if (this == &other) {
        return *this;
    }

    _config = other._config;
    _request = other._request;
    _response = other._response;
    _socket = other._socket;
    _read_buffer = other._read_buffer;
    _read_index = other._read_index;
    _write_buffer = other._write_buffer;
    _write_index = other._write_index;

    return *this;
}

Connection::~Connection() {}

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
    std::cout << "[CONN " << _socket << "] sent " << n << " bytes" << std::endl;
    if (n == 0)
        return 0;
    else if (n < 0) {
        std::cerr << "[Connection::send_data] send: " << strerror(errno) << std::endl;
        return -1;
    }

    // update write index with real bytes sent (can be lower than chunk)
    _write_index += static_cast<size_t>(n);

    // cleanup
    if (_write_index >= _write_buffer.size()) {
        _write_buffer.clear();
        _write_index = 0;
    }
    std::cout << "[CONN " << _socket << "] write complete" << std::endl;
    return n;
}

ssize_t Connection::receive_data() {
    char    buffer[RECV_SIZE];
    ssize_t total = 0;

    while (true) {
        ssize_t n = recv(_socket, buffer, sizeof(buffer), 0);
        if (n > 0) {
            std::cout << "[CONN " << _socket << "] buffer:" << std::endl;
            std::cout.write(buffer, n);
            std::cout << std::endl;
            _read_buffer.append(buffer, static_cast<size_t>(n));
            total += n;
        } else if (n == 0) {
            // closing client
            std::cout << "[CONN " << _socket << "] client closed recv0" << std::endl;
            return 0;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;  // nothing to read

            std::cerr << "[Connection::receive_data] recv: " << strerror(errno) << std::endl;
            return -1;
        }
        if (total > 0)
            std::cout << "[CONN " << _socket << "] received " << n << " bytes" << std::endl;
    }
    if (total > 0) return total;
    return -1;
}

Status_Parsing Connection::parse_request() {
    return parse(_read_buffer, _read_index, _request);
}

void Connection::queue_write(const std::string& data) {
    _write_buffer += data;
}

bool Connection::has_pending_write() const {
    return _write_index < _write_buffer.size();
}

void Connection::set_config(const Config_Server* const config) {
    assert(config && "Config_Server pointer");

    _config = config;
}
