#include "ConnHandler.hpp"

#include <sys/epoll.h>

#include <cstdio>
#include <iostream>

#include "ConnectionManager.hpp"

static std::string hello_response() {
    const std::string body = "Hello\n";
    char              lenbuf[32];

    std::sprintf(lenbuf, "%lu", (unsigned long)body.size());

    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: " +
           std::string(lenbuf) +
           "\r\n"
           "Connection: close\r\n"
           "\r\n" +
           body;
}

ConnHandler::ConnHandler(const Config_Server* srv, int client_fd)
    : _fd(client_fd), _conn(srv, client_fd) {}

ConnHandler::~ConnHandler() {}

int ConnHandler::fd() const {
    return _fd;
}

uint32_t ConnHandler::interests() const {
    // EPOLLOUT only when something must be sent back, else stay ready to listen
    if (_conn.has_pending_write())
        return EPOLLIN | EPOLLOUT;
    else
        return EPOLLIN;
}

bool ConnHandler::handle_event(ConnectionManager& manager, uint32_t events) {
    (void)manager;

    if (events & (EPOLLERR | EPOLLHUP)) {
        std::cerr << "[CONN " << _fd << "] Error: Wrong epoll event";
        return false;
    }

    if (events & EPOLLIN) {
        std::cout << "[CONN " << _fd << "] EPOLLIN" << std::endl;
        ssize_t n = _conn.receive_data();
        if (n < 0) return false;

        // Parse request from _conn.read_buffer() and fill _conn.request()

        // once request parsing is done (if statement ?), fill the response object, serialize it and
        // send it for process.

        // temp, auto hello reponse on any curl.
        if (!_conn.has_pending_write()) {
            _conn.queue_write(hello_response());
        }
    }
    if ((events & EPOLLOUT) && _conn.has_pending_write()) {
        _conn.send_data();
        // temp, close connection once all data was sent
        if (!_conn.has_pending_write()) return false;
    }

    return true;  // keeps connection
}
