#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <sys/types.h>

#include "Request.hpp"
#include "Response.hpp"
#include "config.hpp"

// How much data we should receive from a socket at a time
#define RECV_SIZE 1024
// How much data we should send through a socket at a time
#define SEND_SIZE 1024

class Connection {
public:
    Connection(const Config_Server* const, int socket);
    Connection(const Connection&);
    const Connection& operator=(const Connection&);
    ~Connection();

    const Request&  request() const;
    const Response& response() const;

    ssize_t send_data();
    ssize_t receive_data();

    void set_config(const Config_Server* const);
    void queue_write(const std::string& data);
    bool has_pending_write() const;

private:
    const Config_Server* _config;

    Request  _request;
    Response _response;

    int _socket;

    std::string _read_buffer;
    size_t      _read_index;
    std::string _write_buffer;
    size_t      _write_index;
};

#endif
