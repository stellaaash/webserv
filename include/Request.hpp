#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <cstddef>
#include <string>

#include "HttpMessage.hpp"
#include "config.hpp"

// TODO Move ParsingStatus to a parsing.hpp header once parsing is done
enum ParsingStatus { EMPTY, REQUEST_LINE, HEADERS, BODY, PARSED, ERROR };

/**
 * @brief Represents a request issued by an active connection.
 */
class Request : public HttpMessage {
public:
    Request();
    Request(const ConfigLocation* const, HttpMethod);
    ~Request();

    HttpMethod         method() const;
    ParsingStatus      status() const;
    const std::string& target() const;
    size_t             content_length() const;
    size_t             client_max_body_size() const;
    size_t             body_received() const;
    HttpCode           error_status() const;

    bool               is_body_spooled() const;
    const std::string& body_path() const;

    void set_config(const ConfigLocation* const);
    void set_method(HttpMethod);
    void set_status(ParsingStatus);
    void set_target(const std::string&);
    void set_content_length(size_t);
    void set_client_max_body_size(size_t);
    void set_error_status(HttpCode);

    bool append_body_chunk(const char* data, size_t len);

private:
    Request(const Request&);
    const Request& operator=(const Request&);

    bool open_temp_body_file();
    bool flush_memory_body_to_file();
    void cleanup_temp_file();

    const ConfigLocation* _config;

    std::string   _target;
    size_t        _content_length;
    size_t        _client_max_body_size;
    size_t        _body_received;
    HttpMethod    _method;
    ParsingStatus _status;
    HttpCode      _error_status;

    // Large bodies, over 64Kb
    size_t      _spool_threshold;
    bool        _is_body_spooled;
    int         _body_fd;
    std::string _body_path;
};

ParsingStatus parse(const ConfigServer&, std::string& read_buffer, size_t& read_index, Request&);

#endif
