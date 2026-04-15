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
#include "Response.hpp"
#include "config.hpp"

std::string error_response(HttpCode code) {
    std::string reason;

    switch (code) {
        case 400:
            reason = "Bad Request";
            break;
        case 404:
            reason = "Not Found";
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

    std::ostringstream body_stream;
    body_stream << code << " " << reason << "\n";
    std::string body = body_stream.str();

    std::ostringstream response;
    response << "HTTP/1.1 " << code << " " << reason << "\r\n"
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
    if (_conn.has_pending_write() || _conn.response().status() != RS_EMPTY)
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
 * @brief `true` to signify to keep the connection, `false` if the connection needs to be closed
 * because of an unrecoverable error.
 */
bool ConnectionHandler::handle_event(ConnectionManager& manager, uint32_t events) {
    (void)manager;

    if (events & (EPOLLERR | EPOLLHUP)) {
        Logger(LOG_ERROR) << "[CONN " << _fd << "] Error: Wrong epoll event";
        return false;
    }

    const Request&  request = _conn.request();
    const Response& response = _conn.response();

    if (events & EPOLLIN) {
        Logger(LOG_DEBUG) << "[CONN " << _fd << "] Received data";
        ssize_t n = _conn.receive_data();

        if (n <= 0) return false;

        if (_conn.request().status() != PARSED) {
            ParsingStatus r = _conn.parse_request();
            if (r == ERROR) {
                HttpCode code = request.error_status();
                _conn.queue_write(
                    error_response(code));  // TODO Reconsider whether error_response is a good call
                                            // or if even those erros should go through RS_ERROR
                _conn.send_data();
                return false;
            }
        }
        // If the request has been fully parsed, we're ready to start processing
        if (request.status() == PARSED) {
            log_request(request);
            _conn.process_request();
            log_response(response);
            if (response.status() == RS_ERROR) {
                _conn.queue_write(error_response(response.code()));
                _conn.send_data();
                return false;  // TODO We should only close connections when we can't determine the
                               // end of a request
            }
        }
    }

    if (request.status() == PARSED && !_conn.has_pending_write()) {
        if (response.status() == RS_EMPTY)
            _conn.append_head();
        else if (response.status() == RS_HEAD) {
            _conn.append_body_chunk();
        }
    }

    if (_conn.has_pending_write() && events & EPOLLOUT) {
        Logger(LOG_DEBUG) << "[CONN " << _fd << "] Sent data";
        _conn.send_data();
    }
    if (_conn.bytes_sent() == _conn.total_bytes() && response.status() == RS_SENT) {
        // Reset Request and Response once everything was sent to the client
        _conn.reset_request();
        _conn.reset_response();
    }

    return true;  // keeps connection
}
