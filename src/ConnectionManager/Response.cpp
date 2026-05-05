#include "Response.hpp"

#include <unistd.h>

#include <cassert>
#include <cstdlib>
#include <sstream>
#include <string>

#include "HttpMessage.hpp"
#include "config.hpp"

Response::Response()
    : HttpMessage(),
      _code(0),
      _response_string(),
      _status(RES_EMPTY),
      _fd(-1),
      _body_bytes_sent(0) {
    set_version(1, 1);
}

Response::Response(const Response& other)
    : HttpMessage(other),
      _code(other._code),
      _response_string(other._response_string),
      _status(other._status),
      _body_bytes_sent(other._body_bytes_sent) {
    _fd = dup(other.fd());
}

const Response& Response::operator=(const Response& other) {
    if (this == &other) return *this;

    HttpMessage::operator=(other);

    _code = other._code;
    _response_string = other._response_string;
    _status = other._status;
    _body_bytes_sent = other._body_bytes_sent;

    if (_fd >= 0) close(_fd);
    _fd = dup(other.fd());

    return *this;
}

Response::~Response() {
    if (_fd >= 0) close(_fd);
}

HttpCode Response::code() const {
    return _code;
}

const std::string& Response::response_string() const {
    return _response_string;
}

ResponseStatus Response::status() const {
    return _status;
}

int Response::fd() const {
    return _fd;
}

size_t Response::body_bytes_sent() const {
    return _body_bytes_sent;
}

/**
 * @brief Checks whether the Reponse has an HTTP status code denoting an error.
 * Such statuses must be between 400 and 599 inclusive.
 */
bool Response::is_error() const {
    return (_code >= 400 && _code <= 599);
}

void Response::set_code(HttpCode code) {
    assert(code >= 100 && code <= 599 && "Correct HTTP Code");
    _code = code;
}

void Response::set_response_string(const std::string& response_string) {
    _response_string = response_string;
}

void Response::set_status(ResponseStatus status) {
    _status = status;
}

void Response::set_fd(int fd) {
    _fd = fd;
}

void Response::set_body_bytes_sent(size_t body_bytes_sent) {
    _body_bytes_sent = body_bytes_sent;
}

/**
 * @brief Serialize the Response in order to be able to send it through a socket
 * directly.
 */
std::string Response::serialize() const {
    assert(_code != 0 && "Response ready");

    std::stringstream serialized;

    serialized << "HTTP/" << major_version() << "." << minor_version() << " " << code() << " "
               << response_string() << "\r\n";

    for (HeaderIterator h = headers_begin(); h != headers_end(); ++h) {
        serialized << h->first << ": " << h->second << "\r\n";
    }

    serialized << "\r\n";

    return serialized.str();
}

/**
 * @brief Converts an HTTP status code into an appropriate reason string.
 */
static std::string code_to_string(HttpCode code) {
    switch (code) {
        case 400:
            return "Bad Request";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        case 408:
            return "Connection timeout";
        case 409:
            return "Conflict";
        case 411:
            return "Length Required";
        case 413:
            return "Payload Too Large";
        case 414:
            return "URI Too Long";
        case 431:
            return "Request Header Fields Too Large";
        case 501:
            return "Not Implemented";
        case 505:
            return "HTTP Version Not Supported";
        default:
            return "Error";
    }
}

/**
 * @brief Creates a Response object representing an error.
 */
Response error_response(HttpCode code, bool close) {
    Response result;

    result.set_version(1, 1);
    result.set_code(code);
    result.set_response_string(code_to_string(code));
    result.append_body(result.response_string());
    result.append_body("\r\n");

    std::stringstream stream;
    stream << result.body().size();
    result.set_header("Content-Length", stream.str());
    result.set_header("Content-Type", "text/html");
    if (close == true)
        result.set_header("Connection", "close");
    else
        result.set_header("Connection", "keep-alive");
    return result;
}
