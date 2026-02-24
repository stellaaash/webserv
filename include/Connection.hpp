#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "Request.hpp"
#include "Response.hpp"
#include "config.hpp"

class Connection {
public:
    Connection(const Config_Server* const, int socket);
    ~Connection();

    const Request&  request() const;
    const Response& response() const;

    // TODO Send and receive functions

    void set_config(const Config_Server* const);

private:
    const Config_Server* _config;

    Request  _request;
    Response _response;

    int _socket;
    // TODO Read and write buffers? How?
    // Don't forget the index telling us where the unprocessed data starts
    // That way we don't have to flush the string every time we read from it
};

#endif
