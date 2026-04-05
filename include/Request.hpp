#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <cstddef>
#include <string>

#include "HTTP_Message.hpp"
#include "config.hpp"

// TODO Move Status_Parsing to a parsing.hpp header once parsing is done
enum Status_Parsing { EMPTY, REQUEST_LINE, HEADERS, BODY, PARSED, ERROR };

/**
 * @brief Represents a request issued by an active connection.
 */
class Request : public HTTP_Message {
public:
    Request();
    Request(const Config_Location* const, HTTP_Method);

private:
    Request(const Request&);
    const Request& operator=(const Request&);

public:
    ~Request();

    HTTP_Method        method() const;
    Status_Parsing     status() const;
    const std::string& target() const;
    size_t             content_length() const;
    size_t             client_max_body_size() const;
    size_t             body_received() const;
    HTTP_Code          error_status() const;

    bool               is_body_spooled() const;
    const std::string& body_path() const;

    void set_config(const Config_Location* const);
    void set_method(HTTP_Method);
    void set_status(Status_Parsing);
    void set_target(const std::string&);
    void set_content_length(size_t);
    void set_client_max_body_size(size_t);
    void set_error_status(HTTP_Code);

    bool append_body_chunk(const char* data, size_t len);

private:
    bool open_temp_body_file();
    bool flush_memory_body_to_file();
    bool write_all(int fd, const char* data, size_t len);
    void cleanup_temp_file();

private:
    const Config_Location* _config;

    std::string    _target;
    size_t         _content_length;
    size_t         _client_max_body_size;
    size_t         _body_received;
    HTTP_Method    _method;
    Status_Parsing _status;
    HTTP_Code      _error_status;

    // Large bodies, over 64Kb
    size_t      _spool_threshold;
    bool        _is_body_spooled;
    int         _body_fd;
    std::string _body_path;
};

Status_Parsing parse(std::string& read_buffer, size_t& read_index, Request& request);

#endif
