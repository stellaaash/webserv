#include "Connection.hpp"

#include "Request.hpp"
#include "Response.hpp"
#include "config.hpp"

Connection::Connection(const Config_Server* config, int socket)
    : _config(config), _socket(socket) {}

Connection::~Connection() {}

const Request& Connection::request() const {
    return _request;
}

const Response& Connection::response() const {
    return _response;
}
