#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <cstddef>
#include <string>

#include "HttpMessage.hpp"
#include "config.hpp"

enum RequestStatus {
    REQ_EMPTY,         // Nothing received yet
    REQ_REQUEST_LINE,  // Received request line
    REQ_HEADERS,       // Received headers
    REQ_BODY,          // Received the full body
    REQ_PARSED,        // Ready to be processed
    REQ_PROCESSED,     // Fully processed, Response created
    REQ_ERROR          // Error during parsing
};

/**
 * @brief Represents a request issued by an active connection.
 */
class Request : public HttpMessage {
public:
    Request();
    Request(const Request&);
    const Request& operator=(const Request&);
    Request(const ConfigLocation* const, HttpMethod);
    ~Request();

    const ConfigLocation& config() const;
    HttpMethod            method() const;
    RequestStatus         status() const;
    const std::string&    target() const;
    size_t                content_length() const;
    size_t                client_max_body_size() const;
    size_t                body_received() const;
    HttpCode              error_status() const;

    bool               is_body_spooled() const;
    const std::string& body_path() const;

    void set_config(const ConfigLocation* const);
    void set_method(HttpMethod);
    void set_status(RequestStatus);
    void set_target(const std::string&);
    void set_content_length(size_t);
    void set_client_max_body_size(size_t);
    void set_error_status(HttpCode);

    bool append_body_chunk(const char* data, size_t len);

private:
    bool open_temp_body_file();
    bool flush_memory_body_to_file();
    void cleanup_temp_file();

    const ConfigLocation* _config;

    std::string   _target;
    size_t        _content_length;
    size_t        _client_max_body_size;
    size_t        _body_received;
    HttpMethod    _method;
    RequestStatus _status;
    HttpCode      _error_status;

    // Large bodies, over 64Kb
    size_t      _spool_threshold;
    bool        _is_body_spooled;
    int         _body_fd;
    std::string _body_path;
};

#endif
