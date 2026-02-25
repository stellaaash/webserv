#include "Connection.hpp"

#include <sys/socket.h>
#include <sys/types.h>

#include <cassert>

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

    _working_read_buffer = new char[RECV_SIZE];
    _working_write_buffer = new char[SEND_SIZE];
}

Connection::~Connection() {
    delete[] _working_read_buffer;
    delete[] _working_write_buffer;
}

const Request& Connection::request() const {
    return _request;
}

const Response& Connection::response() const {
    return _response;
}

/**
 * @brief Returns a pointer to the first unprocessed char in the read buffer.
 */
const char* Connection::read_data() const {
    return _read_buffer.c_str() + _read_index;
}

/**
 * @brief Returns a pointer to the first unprocessed char in the write buffer.
 */
const char* Connection::write_data() const {
    return _write_buffer.c_str() + _write_index;
}

/**
 * @brief Send a chunk of data in the write buffer through the connection's socket.
 */
size_t Connection::send_data() {
    ssize_t sent_bytes = send(_socket, write_data(), SEND_SIZE, 0);

    if (sent_bytes < 0) {
        throw;
        // TODO Log error
    }

    // We cast it to unsigned because at this point it has to be positive
    return static_cast<size_t>(sent_bytes);
}

size_t Connection::receive_data() {
    ssize_t received_bytes = recv(_socket, _working_read_buffer, RECV_SIZE, 0);

    if (received_bytes < 0) {
        throw;
        // TODO Log error
    }

    // We cast it to unsigned because at this point it has to be positive
    _read_buffer.append(_working_read_buffer, static_cast<size_t>(received_bytes));

    return static_cast<size_t>(received_bytes);
}

void Connection::set_config(const Config_Server* const config) {
    assert(config && "Config_Server pointer");

    _config = config;
}
