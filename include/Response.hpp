#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>

#include "HttpMessage.hpp"
#include "config.hpp"

/**
 * @brief This status signifies what has been appended to the write_buffer of the connection, in
 * effect what has been sent to the client already. We need to keep track of this to only send the
 * response line and headers (RES_HEAD) once, and only after that the body.
 */
enum ResponseStatus { RES_EMPTY, RES_HEAD, RES_SENT };

class Response : public HttpMessage {
public:
    Response();
    Response(const Response&);
    const Response& operator=(const Response&);
    ~Response();

    HttpCode           code() const;
    const std::string& response_string() const;
    ResponseStatus     status() const;
    int                fd() const;
    bool               is_error() const;

    void set_code(HttpCode);
    void set_response_string(const std::string&);
    void set_status(ResponseStatus);
    void set_fd(int);

    std::string serialize() const;

private:
    HttpCode       _code;
    std::string    _response_string;
    ResponseStatus _status;
    int            _fd;
};

Response error_response(HttpCode, bool close);

#endif
