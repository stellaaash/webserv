#include "ConnectionHandler.hpp"

#include <sys/epoll.h>

#include <cstdio>
#include <iostream>
#include <sstream>

#include "ConnectionManager.hpp"
#include "Request.hpp"
#include "config.hpp"

static std::string error_response(HttpCode code) {
    std::string reason;

    switch (code) {
        case 400:
            reason = "Bad Request";
            break;
        case 405:
            reason = "Method Not Allowed";
            break;
        case 411:
            reason = "Length Required";
            break;
        case 413:
            reason = "Payload Too Large";
            break;
        case 414:
            reason = "URI Too Long";
            break;
        case 431:
            reason = "Request Header Fields Too Large";
            break;
        case 501:
            reason = "Not Implemented";
            break;
        case 505:
            reason = "HTTP Version Not Supported";
            break;
        default:
            reason = "Error";
            break;
    }

    int code_int = static_cast<int>(code);

    std::ostringstream body_stream;
    body_stream << code_int << " " << reason << "\n";
    std::string body = body_stream.str();

    std::ostringstream response;
    response << "HTTP/1.1 " << code_int << " " << reason << "\r\n"
             << "Content-Type: text/plain\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << body;

    return response.str();
}

static std::string hello_response() {
    const std::string body = "Hello\n";

    std::ostringstream oss;
    oss << body.size();

    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: " +
           oss.str() +
           "\r\n"
           "Connection: keep-alive\r\n"
           "\r\n" +
           body;
}

ConnectionHandler::ConnectionHandler(const ConfigServer* srv, int client_fd)
    : _fd(client_fd),
      _conn(srv, client_fd),
      _last_activity(std::time(NULL)),
      _timeout(static_cast<long>(srv->timeout)) {}

ConnectionHandler::~ConnectionHandler() {}

int ConnectionHandler::fd() const {
    return _fd;
}

uint32_t ConnectionHandler::interests() const {
    // EPOLLOUT only when something must be sent back, else stay ready to listen
    if (_conn.has_pending_write())
        return EPOLLIN | EPOLLOUT;
    else
        return EPOLLIN;
}

bool ConnectionHandler::is_timed_out() const {
    std::cout << _timeout << std::endl;
    return (std::time(NULL) - _last_activity) > _timeout;
}

bool ConnectionHandler::handle_event(ConnectionManager& manager, uint32_t events) {
    (void)manager;

    if (events & (EPOLLERR | EPOLLHUP)) {
        std::cerr << "[CONN " << _fd << "] Error: Wrong epoll event";
        return false;
    }

    if (events & EPOLLIN) {
        std::cout << "[CONN " << _fd << "] EPOLLIN" << std::endl;
        ssize_t n = _conn.receive_data();

        if (n < 0) return false;
        if (n == 0) return false;  // temp to avoid infinite calls when closed by client

        ParsingStatus r = _conn.parse_request();
        if (r == ERROR) {
            HttpCode code = _conn.request().error_status();
            _conn.queue_write(error_response(code));
            _conn.send_data();
            return false;
        }
        if (r == PARSED) {
            const Request& req = _conn.request();
            std::cout << "----- [REQUEST] -----\n";
            std::cout << "Request Status:" << req.status() << "\n";
            std::cout << "Method: " << req.method() << "\n";
            std::cout << "Target: " << req.target() << "\n";
            std::cout << "Body received: [" << req.body_received() << "]\n";
            std::cout << "Is body spooled: [" << req.is_body_spooled() << "]\n";
            if (req.is_body_spooled()) {
                std::cout << "Body path: " << req.body_path() << "\n";
            } else {
                std::cout << "Body size (RAM): " << req.body().size() << "\n";
            }
            std::cout << "----- [HEADERS] -----\n";
            for (HttpMessage::HeaderIterator it = req.headers_begin(); it != req.headers_end();
                 ++it) {
                std::cout << it->first << ": " << it->second << "\n";
            }
            std::cout << "----- [REQ END] -----" << "\n\n\n";
        }

        if (!_conn.has_pending_write() && r == PARSED) {
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
