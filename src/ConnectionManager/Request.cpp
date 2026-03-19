#include "Request.hpp"

#include <cassert>

#include "HTTP_Message.hpp"
#include "config.hpp"

Request::Request()
    : _config(NULL), _target(""), _content_length(0), _method(UNDEFINED), _status(EMPTY) {}

Request::Request(const Config_Location* const config, HTTP_Method method)
    : _config(config), _target(""), _content_length(0), _method(method), _status(EMPTY) {
    assert(config && "Config_Location pointer");
}

Request::Request(const Request& other)
    : HTTP_Message(other),
      _config(other._config),
      _target(other._target),
      _content_length(other._content_length),
      _method(other._method),
      _status(other._status) {}

const Request& Request::operator=(const Request& other) {
    if (this == &other) return *this;

    HTTP_Message::operator=(other);
    _config = other._config;
    _target = other._target;
    _content_length = other._content_length;
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

const std::string& Request::target() const {
    return _target;
}

size_t Request::content_length() const {
    return _content_length;
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

void Request::set_target(const std::string& target) {
    _target = target;
}

void Request::set_content_length(size_t content_length) {
    _content_length = content_length;
}
