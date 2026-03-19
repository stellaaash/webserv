#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <cstddef>
#include <string>

#include "HTTP_Message.hpp"
#include "config.hpp"

// TODO Move Status_Parsing to a parsing.hpp header once parsing is done
enum Status_Parsing { EMPTY, REQUEST_LINE, HEADERS, BODY, PARSED };

/**
 * @brief Represents a request issued by an active connection.
 */
class Request : public HTTP_Message {
public:
    Request();
    Request(const Config_Location* const, HTTP_Method);
    Request(const Request&);
    const Request& operator=(const Request&);
    ~Request();

    HTTP_Method        method() const;
    Status_Parsing     status() const;
    const std::string& target() const;
    size_t             content_length() const;

    void set_config(const Config_Location* const);
    void set_method(HTTP_Method);
    void set_status(Status_Parsing);
    void set_target(const std::string&);
    void set_content_length(size_t);

private:
    const Config_Location* _config;

    std::string    _target;
    size_t         _content_length;
    HTTP_Method    _method;
    Status_Parsing _status;
};

#endif
