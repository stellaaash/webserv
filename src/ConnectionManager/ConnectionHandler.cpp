#include "ConnectionHandler.hpp"

#include <sys/epoll.h>

#include <cstdio>

#include "ConnectionManager.hpp"
#include "HttpMessage.hpp"
#include "Logger.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "cgi.hpp"
#include "config.hpp"

/**
 * @brief Prints a request's status, as well as its body information and headers.
 */
static void log_request(const Request& request) {
    Logger(LOG_DEBUG) << "----- [REQUEST] -----";
    Logger(LOG_DEBUG) << "Request Status:" << request.status();
    Logger(LOG_DEBUG) << "Method: " << request.method();
    Logger(LOG_DEBUG) << "Target: " << request.target();
    if (request.config()) Logger(LOG_DEBUG) << "Matched location: " << request.config()->name;
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
      _timeout(static_cast<long>(srv->timeout)),
      _cgi_handler(NULL) {}

ConnectionHandler::~ConnectionHandler() {
    // Cleanly abort Cgi to avoid memory corruption
    if (_cgi_handler) {
        _cgi_handler->detach_client();
        _cgi_handler->abort_cgi();
        _cgi_handler = NULL;
    }
}

int ConnectionHandler::fd() const {
    return _fd;
}

void ConnectionHandler::finish_cgi(const std::string& output, int cgi_status, int exit_code) {
    if (cgi_status == CGI_TIMEOUT) {
        _conn.set_response(error_response(504, true));
    } else if (cgi_status != CGI_OK || exit_code != 0) {
        _conn.set_response(error_response(500, true));
    } else {
        // TODO: Cgi returns headers in output and must be parsed here.
        Response response;

        response.set_code(200);
        response.set_header("Content-Type", "text/plain");
        response.set_header("Content-Length", to_string_size(output.size()));
        response.append_body(output);

        _conn.set_response(response);
    }

    Request done = _conn.request();
    done.set_status(REQ_PROCESSED);
    _conn.set_request(done);

    if (!_conn.has_pending_write()) {
        _conn.queue_head();
        _conn.queue_body_chunk();
    }

    if (_conn.has_pending_write()) _conn.send_data();
}

uint32_t ConnectionHandler::interests() const {
    // EPOLLOUT only when something must be sent back, else stay ready to listen
    if (_conn.response().status() != RES_EMPTY)
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
    if (_conn.request().status() == REQ_PARSED)
        return false;  // If parsed status, CGI can timeout but not the connection itself

    return (std::time(NULL) - _last_activity) > _timeout;
}

/**
 * @brief After a conneciton has timed out, this member function will generate and send a 408 error
 * response before the Connection Manager closes the connection outright.
 */
void ConnectionHandler::timeout_connection() {
    Logger(LOG_DEBUG) << "[!] - Timing connection out!";
    _conn.set_response(error_response(408, true));
    _conn.queue_write(_conn.response().serialize());
    _conn.queue_write(_conn.response().body());
    _conn.send_data();
}

void ConnectionHandler::clear_cgi_handler() {
    _cgi_handler = NULL;
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
    Logger(LOG_DEBUG) << "[!] - Response status: " << response.status();

    if (events & (EPOLLERR | EPOLLHUP)) {
        Logger(LOG_ERROR) << "[CONN " << _fd << "] Error: Wrong epoll event";
        return false;
    }

    // Parse request on incoming data
    if (events & EPOLLIN) {
        ssize_t n = _conn.receive_data();
        if (n <= 0) return false;

        _conn.parse_request();
        if (request.status() == REQ_PARSED || request.status() == REQ_ERROR) log_request(request);
    }

    // If request has been fully parsed, process it
    if (request.status() == REQ_PARSED) {
        _conn.process_request();
        log_response(response);
    } else if (request.status() == REQ_ERROR) {
        _conn.set_response(error_response(request.error_status(), true));
        log_response(response);
    }

    // Send the response once it is ready
    if (!_conn.has_pending_write()) {
        if (request.status() == REQ_PROCESSED || request.status() == REQ_ERROR) {
            if (response.status() == RES_EMPTY) _conn.queue_head();
            if (response.status() == RES_HEAD) _conn.queue_body_chunk();
        }
    }

    if (_conn.has_pending_write()) _conn.send_data();

    // Reset request and response objects once everything was sent
    if (response.status() == RES_SENT && _conn.has_pending_write() == false) {
        bool                        must_close = false;
        HttpMessage::HeaderIterator connection = response.header("Connection");
        if (connection != response.headers_end() && connection->second == "close")
            must_close = true;  // If "Connection: close", close connection after sending

        _conn.set_request(Request());
        _conn.set_response(Response());
        _cgi_handler = NULL;
        Logger(LOG_DEBUG) << "[!] - Reset request and response objects";
        if (must_close) return false;
    }

    return true;
}
