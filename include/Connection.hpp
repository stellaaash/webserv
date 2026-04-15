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

    void reset_request();
    void reset_response();

    ssize_t       send_data();
    ssize_t       receive_data();
    ParsingStatus parse_request();
    void          process_request();
    void          append_head();
    void          append_body_chunk();

    void set_config(const ConfigServer* const);
    void queue_write(const std::string& data);
    bool has_pending_write() const;

    size_t bytes_sent() const;
    size_t total_bytes() const;

private:
    Connection(const Connection&);
    Connection& operator=(const Connection&);

    void shrink_read_buffer();

    void process_get_request(const FilePath& resource_path);
    void process_post_request();
    void process_delete_request();

    const ConfigServer* _config;

    Request  _request;
    Response _response;

    int _socket;

    std::string _read_buffer;
    size_t      _read_index;
    std::string _write_buffer;
    size_t      _write_index;

    // Keep track of how much of the response and its body have been sent
    size_t _bytes_sent;
    size_t _total_bytes;
};

#endif
