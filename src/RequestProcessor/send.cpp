#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <cstring>

#include "Connection.hpp"
#include "Logger.hpp"
#include "Response.hpp"

/**
 * @brief Serializes the Reponse head (response line + headers) and appends it to the connection's
 * write_buffer.
 */
void Connection::queue_head() {
    assert(_response.status() == RES_EMPTY);

    queue_write(_response.serialize());
    _response.set_status(RES_HEAD);
}

/**
 * @brief Appends a chunk of body to the connection's write_buffer.
 */
void Connection::queue_body_chunk() {
    assert(_response.status() == RES_HEAD);

    if (_response.fd() >= 0) {
        char    buffer[SEND_SIZE];
        ssize_t read_bytes = read(_response.fd(), buffer, SEND_SIZE);
        if (read_bytes < 0) {
            Logger(LOG_ERROR) << "[Connection::queue_body_chunk] read: " << strerror(errno);
            _response.set_code(500);
            return;
        } else if (read_bytes < SEND_SIZE) {
            Logger(LOG_DEBUG) << "[!] - Response sent!";
            _response.set_status(RES_SENT);
        }
        queue_write(std::string(buffer, static_cast<size_t>(read_bytes)));
    } else if (_response.body().empty() == false) {
        const size_t start_bytes = _response.body_bytes_sent();
        const size_t remaining_bytes = _response.body().size() - start_bytes;
        const size_t chunk_size = SEND_SIZE < remaining_bytes ? SEND_SIZE : remaining_bytes;

        queue_write(_response.body().substr(start_bytes, chunk_size));
        _response.set_body_bytes_sent(start_bytes + chunk_size);
        Logger(LOG_DEBUG) << "[!] - Response sent!";
        if (_response.body().length() == _response.body_bytes_sent())
            _response.set_status(RES_SENT);
    }
}
