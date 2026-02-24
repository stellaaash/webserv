#include "Connection.hpp"

#include <cassert>

#include "Request.hpp"
#include "Response.hpp"
#include "config.hpp"

Connection::Connection(const Config_Server* config, int socket) : _config(config), _socket(socket) {
    assert(config && "Config_Server pointer");
    assert(socket > 2 && "Valid Socket Number");
}

Connection::~Connection() {}

const Request& Connection::request() const {
    return _request;
}

const Response& Connection::response() const {
    return _response;
}
