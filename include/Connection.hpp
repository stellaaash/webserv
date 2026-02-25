#ifndef CONNECTION_HPP
#define CONNECTION_HPP

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
    ~Connection();

    const Request&  request() const;
    const Response& response() const;
    const char*     read_data() const;
    const char*     write_data() const;

    size_t send_data();
    size_t receive_data();

    void set_config(const Config_Server* const);

private:
    const Config_Server* _config;

    Request  _request;
    Response _response;

    int _socket;

    char*       _working_read_buffer;
    std::string _read_buffer;
    size_t      _read_index;
    char*       _working_write_buffer;  // TODO Potentially useless
    std::string _write_buffer;
    size_t      _write_index;
};

#endif
