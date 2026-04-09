#include "Response.hpp"

#include <cassert>
#include <cstdlib>
#include <sstream>
#include <string>

#include "HttpMessage.hpp"
#include "config.hpp"

Response::Response() : _code(0), _response_string() {}

Response::Response(const Response& other)
    : HttpMessage(other), _code(other._code), _response_string(other._response_string) {}

const Response& Response::operator=(const Response& other) {
    if (this == &other) return *this;

    _code = other._code;
    _response_string = other._response_string;

    return *this;
}

Response::~Response() {}

int Response::fd() const {
    return _fd;
}

HttpCode Response::code() const {
    return _code;
}

const std::string& Response::response_string() const {
    return _response_string;
}

void Response::set_fd(int fd) {
    _fd = fd;
}

void Response::set_code(HttpCode code) {
    assert(code >= 100 && code <= 599 && "Correct HTTP Code");
    _code = code;
}

void Response::set_response_string(const std::string& response_string) {
    _response_string = response_string;
}

/**
 * @brief Serialize the Response in order to be able to send it through a socket
 * directly.
 */
std::string Response::serialize() const {
    assert(_response_string.empty() == false && "Response ready");

    std::stringstream serialized;

    serialized << "HTTP/" << major_version() << "." << minor_version() << " " << code() << " "
               << response_string();

    // TODO Add headers

    serialized << "\r\n\r\n";

    return serialized.str();
}
