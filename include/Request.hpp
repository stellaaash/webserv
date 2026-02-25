#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "HTTP_Message.hpp"
#include "config.hpp"

enum HTTP_Method { UNDEFINED, GET, POST, DELETE };
// TODO Move Status_Parsing to a parsing.hpp header once parsing is done
enum Status_Parsing { EMPTY, REQUEST_LINE, HEADERS, BODY, PARSED };

/**
 * @brief Represents a request issued by an active connection.
 */
class Request : public HTTP_Message {
public:
    Request();
    Request(const Config_Location* const, HTTP_Method);
    ~Request();

    HTTP_Method    method() const;
    Status_Parsing status() const;

    void set_config(const Config_Location* const);
    void set_method(HTTP_Method);
    void set_status(Status_Parsing);

private:
    const Config_Location* _config;

    HTTP_Method    _method;
    Status_Parsing _status;
};

#endif
