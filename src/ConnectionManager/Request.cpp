#include "Request.hpp"

#include "config.hpp"

Request::Request(const Config_Location& config, HTTP_Method method)
    : _config(config), _method(method), _status(EMPTY) {}

Request::~Request() {}

HTTP_Method Request::method() const {
    return _method;
}

Status_Parsing Request::status() const {
    return _status;
}

void Request::set_method(HTTP_Method method) {
    _method = method;
}

void Request::set_status(Status_Parsing status) {
    _status = status;
}
