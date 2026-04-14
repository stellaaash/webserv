#include "ConnectionHandler.hpp"

#include <sys/epoll.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>

#include "Connection.hpp"
#include "ConnectionManager.hpp"
#include "Logger.hpp"
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

/**
 * @brief Prints a request's status, as well as its body information and headers.
 */
static void log_request(const Request& request) {
    Logger(LOG_DEBUG) << "----- [REQUEST] -----";
    Logger(LOG_DEBUG) << "Request Status:" << request.status();
    Logger(LOG_DEBUG) << "Method: " << request.method();
    Logger(LOG_DEBUG) << "Target: " << request.target();
    Logger(LOG_DEBUG) << "Body received: [" << request.body_received() << "]";
    Logger(LOG_DEBUG) << "Is body spooled: [" << request.is_body_spooled() << "]";
    if (request.is_body_spooled()) {
        Logger(LOG_DEBUG) << "Body path: " << request.body_path();
    } else {
        Logger(LOG_DEBUG) << "Body size (RAM): " << request.body().size();
    }
    Logger(LOG_DEBUG) << "Request location config: " << request.config()->name;
    Logger(LOG_DEBUG) << "----- [HEADERS] -----";
    for (HttpMessage::HeaderIterator it = request.headers_begin(); it != request.headers_end();
         ++it) {
        Logger(LOG_DEBUG) << it->first << ": " << it->second;
    }
    Logger(LOG_DEBUG) << "----- [REQ END] -----\n";
}

/**
 * @brief Prints a response's status, as well as its body information and headers.
 */
static void log_response(const Response& response) {
    Logger(LOG_DEBUG) << "----- [RESPONSE] -----";
    Logger(LOG_DEBUG) << "Code: " << response.code();
    Logger(LOG_DEBUG) << "Response String: " << response.response_string();
    Logger(LOG_DEBUG) << "File Descriptor: " << response.fd();
    Logger(LOG_DEBUG) << "Body length: " << response.body().length();
    Logger(LOG_DEBUG) << "----- [HEADERS] -----";
    for (HttpMessage::HeaderIterator it = response.headers_begin(); it != response.headers_end();
         ++it) {
        Logger(LOG_DEBUG) << it->first << ": " << it->second;
    }
    Logger(LOG_DEBUG) << "----- [RESP END] -----\n";
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
    return (std::time(NULL) - _last_activity) > _timeout;
}

/**
 * @brief This functions handles an event happening over a connection; either data arrived or is
 * ready to be sent.
 *
 * In any case, if any error occurs, or if the data was fully sent or received, it closes the
 * connection by returning `false`. Returning `true` keeps the connection alive upstream.
 */
bool ConnectionHandler::handle_event(ConnectionManager& manager, uint32_t events) {
    (void)manager;

    if (events & (EPOLLERR | EPOLLHUP)) {
        Logger(LOG_ERROR) << "[CONN " << _fd << "] Error: Wrong epoll event";
        return false;
    }

    if (events & EPOLLIN) {
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
        // Stop here if the request hasn't been fully parsed
        if (r != PARSED) return true;

        log_request(_conn.request());

        _conn.process_request();

        const Response& response = _conn.response();

        log_response(response);

        // TODO Will need to not do everything in one go to prevent blocking on processing the
        // request
        if (response.code() >= 400 && response.code() <= 599) {
            _conn.queue_write(error_response(response.code()));
        } else if (!_conn.has_pending_write() && r == PARSED) {
            _conn.queue_write(response.serialize());
            if (response.body().empty() == false) {
                // TODO Generated bodies above SEND_SIZE will block the server
                // This is because this queue_write occurs only when the request first gets parsed
                _conn.queue_write(response.body());
            }
        }
    }

    // Add a chunk to send
    if (_conn.response().fd() >= 0) {
        char    buffer[SEND_SIZE];
        ssize_t read_bytes = read(_conn.response().fd(), buffer, SEND_SIZE);
        if (read_bytes < 0) perror("[handle_event] - read");
        _conn.queue_write(std::string(buffer, static_cast<size_t>(read_bytes)));
    }

    if ((events & EPOLLOUT) && _conn.has_pending_write()) _conn.send_data();

    return true;  // keeps connection
}
