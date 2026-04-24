#include <unistd.h>

#include <cassert>
#include <cstdio>

#include "Connection.hpp"
#include "Response.hpp"

/**
 * @brief Serializes the Reponse head (response line + headers) and appends it to the connection's
 * write_buffer.
 */
void Connection::append_head() {
    assert(_response.status() == RES_EMPTY);

    queue_write(_response.serialize());
    _response.set_status(RES_HEAD);
}

/**
 * @brief Appends a chunk of body to the connection's write_buffer.
 */
void Connection::append_body_chunk() {
    assert(_response.status() == RES_HEAD);

    if (_response.fd() >= 0) {
        char    buffer[SEND_SIZE];
        ssize_t read_bytes = read(_response.fd(), buffer, SEND_SIZE);
        if (read_bytes < 0) {
            perror("[handle_event] - read");
            _response.set_code(500);
            _response.set_status(RES_ERROR);
            return;
        } else if (read_bytes == 0)
            _response.set_status(RES_SENT);
        queue_write(std::string(buffer, static_cast<size_t>(read_bytes)));
    } else if (_response.body().empty() == false) {
        queue_write(_response.body());  // TODO Don't send body in one chunk
        _response.set_status(RES_SENT);
    }
}
