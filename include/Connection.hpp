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
    Connection(const ConfigServer* const, int socket);
    ~Connection();

    const Request&  request() const;
    const Response& response() const;

    ssize_t       send_data();
    ssize_t       receive_data();
    RequestStatus parse_request();
    void          process_request();
    void          queue_head();
    void          queue_body_chunk();

    void set_config(const ConfigServer* const);
    void set_request(const Request&);
    void set_response(const Response&);
    void queue_write(const std::string& data);
    bool has_pending_write() const;

private:
    Connection(const Connection&);
    Connection& operator=(const Connection&);

    void shrink_read_buffer();

    RequestStatus parse_request_line();
    RequestStatus parse_headers();
    RequestStatus parse_body();
    RequestStatus resolve_location();

    void process_get_request(const FilePath&);
    void process_post_request(const FilePath&);
    void process_delete_request(const FilePath&);

    const ConfigServer* _config;

    Request  _request;
    Response _response;

    int _socket;

    std::string _read_buffer;
    size_t      _read_index;
    std::string _write_buffer;
    size_t      _write_index;
};

#endif
