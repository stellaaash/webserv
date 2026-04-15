#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>

#include "HttpMessage.hpp"
#include "config.hpp"

/**
 * @brief This status signifies what has been appended to the write_buffer of the connection, in
 * effect what has been sent to the client already. We need to keep track of this to only send the
 * response line and headers (RS_HEAD) once, and only after that the body.
 */
enum ResponseStatus { RS_EMPTY, RS_HEAD, RS_SENT, RS_ERROR };

class Response : public HttpMessage {
public:
    Response();
    Response(const Response&);
    const Response& operator=(const Response&);
    ~Response();

    int                fd() const;
    HttpCode           code() const;
    const std::string& response_string() const;
    ResponseStatus     status() const;

    void set_fd(int);
    void set_code(HttpCode);
    void set_response_string(const std::string&);
    void set_status(ResponseStatus);

    std::string serialize() const;

private:
    int            _fd;
    HttpCode       _code;
    std::string    _response_string;
    ResponseStatus _status;
};

#endif
