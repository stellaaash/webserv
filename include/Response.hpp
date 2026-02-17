#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>

#include "HTTP_Message.hpp"
#include "config.hpp"

class Response : virtual public HTTP_Message {
public:
    Response();
    ~Response();

    std::string serialize() const;

private:
    HTTP_Code   _code;
    std::string _response_string;
};

#endif
