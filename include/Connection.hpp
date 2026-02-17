#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "Request.hpp"
#include "Response.hpp"
#include "config.hpp"

class Connection {
public:
    Connection(const Config_Server& config);
    ~Connection();

    const Response& response() const;
    const Request&  request() const;

private:
    Request              _request;
    Response             _response;
    const Config_Server& _config;

    // TODO Socket
    // TODO Read buffer? How?
};

#endif
