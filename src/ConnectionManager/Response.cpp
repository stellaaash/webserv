#include "Response.hpp"

#include <cassert>
#include <cstdlib>
#include <sstream>
#include <string>

#include "HttpMessage.hpp"
#include "config.hpp"

Response::Response() : _code(0), _response_string(), _status(RES_EMPTY) {}

Response::Response(const Response& other)
    : HttpMessage(other),
      _code(other._code),
      _response_string(other._response_string),
      _status(other._status) {}

const Response& Response::operator=(const Response& other) {
    if (this == &other) return *this;

    _code = other._code;
    _response_string = other._response_string;
    _status = other._status;

    return *this;
}

Response::~Response() {}

HttpCode Response::code() const {
    return _code;
}

const std::string& Response::response_string() const {
    return _response_string;
}

ResponseStatus Response::status() const {
    return _status;
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
        case 405:
            return "Method Not Allowed";
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

std::string error_response(HttpCode code) {
    std::string reason = code_to_string(code);

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
