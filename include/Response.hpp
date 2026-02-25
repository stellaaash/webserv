#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>

#include "HTTP_Message.hpp"
#include "config.hpp"

class Response : public HTTP_Message {
public:
    Response();
    ~Response();

    HTTP_Code          code() const;
    const std::string& response_string() const;

    void set_code(HTTP_Code);
    void set_response_string(const std::string&);

    std::string serialize() const;

private:
    HTTP_Code   _code;
    std::string _response_string;
};

#endif
