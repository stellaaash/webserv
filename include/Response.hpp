#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>

#include "HttpMessage.hpp"
#include "config.hpp"

class Response : public HttpMessage {
public:
    Response();
    Response(const Response&);
    const Response& operator=(const Response&);
    ~Response();

    int                fd() const;
    HttpCode           code() const;
    const std::string& response_string() const;

    void set_fd(int);
    void set_code(HttpCode);
    void set_response_string(const std::string&);

    std::string serialize() const;

private:
    int         _fd;
    HttpCode    _code;
    std::string _response_string;
};

#endif
