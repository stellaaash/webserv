#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "Request.hpp"
#include "Response.hpp"
#include "config.hpp"

class Connection {
public:
    Connection(const Config_Server& config, int socket);
    ~Connection();

    const Response& response() const;
    const Request&  request() const;

    // TODO Send and receive functions

private:
    Request              _request;
    Response             _response;
    const Config_Server& _config;

    int _socket;
    // TODO Read and write buffers? How?
};

#endif
