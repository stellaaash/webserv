#include "Request.hpp"

#include <cassert>

#include "HTTP_Message.hpp"
#include "config.hpp"

Request::Request() : _config(NULL), _method(UNDEFINED), _status(EMPTY) {}

Request::Request(const Config_Location* const config, HTTP_Method method)
    : _config(config), _method(method), _status(EMPTY) {
    assert(config && "Config_Location pointer");
}

Request::Request(const Request& other)
    : HTTP_Message(other), _config(other._config), _method(other._method), _status(other._status) {}

const Request& Request::operator=(const Request& other) {
    if (this == &other) return *this;

    _config = other._config;
    _method = other._method;
    _status = other._status;

    return *this;
}

Request::~Request() {}

HTTP_Method Request::method() const {
    return _method;
}

Status_Parsing Request::status() const {
    return _status;
}

void Request::set_config(const Config_Location* const config) {
    assert(config && "Config_Server pointer");

    _config = config;
}

void Request::set_method(HTTP_Method method) {
    _method = method;
}

void Request::set_status(Status_Parsing status) {
    assert(status > _status && "Walking back status");  // Status should only progress, not go back
    _status = status;
}
