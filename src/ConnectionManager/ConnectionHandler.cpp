#include "ConnectionHandler.hpp"

#include <sys/epoll.h>

#include <cstdio>
#include <iostream>
#include <sstream>

#include "ConnectionManager.hpp"
#include "Logger.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "config.hpp"

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
    Logger(LOG_DEBUG) << "----- [HEADERS] -----";
    for (HttpMessage::HeaderIterator it = request.headers_begin(); it != request.headers_end();
         ++it) {
        Logger(LOG_DEBUG) << it->first << ": " << it->second;
    }
    Logger(LOG_DEBUG) << "----- [REQ END] -----\n";
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

/**
 * @brief Checks whether a connection has been timed out or not.
 * This is directly based on the `timeout` directive in the server configuration.
 * If this is set to 0 (as is the default), any incoming data that isn't immediately parsed to a
 * full request will result in a connection close.
 */
bool ConnectionHandler::is_timed_out() const {
    return (std::time(NULL) - _last_activity) > _timeout;
}

/**
 * @brief After a conneciton has timed out, this member function will generate and send a 408 error
 * response before the Connection Manager closes the connection outright.
 */
void ConnectionHandler::timeout_connection() {
    Logger(LOG_DEBUG) << "[!] - Timing connection out!";
    _conn.set_response(error_response(408));
    _conn.queue_write(_conn.response().serialize());
    _conn.queue_write(_conn.response().body());
    _conn.send_data();
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
    const Request&  request = _conn.request();
    const Response& response = _conn.response();

    if (events & (EPOLLERR | EPOLLHUP)) {
        Logger(LOG_ERROR) << "[CONN " << _fd << "] Error: Wrong epoll event";
        return false;
    }

    if (events & EPOLLIN) {
        ssize_t n = _conn.receive_data();
        if (n <= 0) return false;

        _conn.parse_request();
    }

    if (request.status() == REQ_PARSED) {
        log_request(request);
        _conn.process_request();
    }

    // Add data to send depending on state
    if (!_conn.has_pending_write()) {
        if (request.status() == REQ_PROCESSED) {
            _conn.queue_write(hello_response());  // Replace with actual response when it's ready
        } else if (request.status() == REQ_ERROR) {
            log_request(request);
            HttpCode code = request.error_status();
            _conn.set_response(error_response(code));
            // TODO Temporary, will have to be swapped with RePro logic
            _conn.queue_write(response.serialize());
            _conn.queue_write(response.body());
        } else if (response.status() == RES_ERROR) {
            // TODO Temporary, will have to be swapped with RePro logic
            _conn.queue_write(response.serialize());
            _conn.queue_write(response.body());
        }
    }

    if (events & EPOLLOUT && _conn.has_pending_write()) _conn.send_data();

    return true;
}
