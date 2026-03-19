#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include <cstddef>
#include <string>

#include "Request.hpp"

class RequestParser {
public:
    static Status_Parsing parse(std::string& read_buffer, size_t& read_index, Request& request);

private:
    RequestParser();
    RequestParser(const RequestParser&);
    RequestParser& operator=(const RequestParser&);

    static Status_Parsing parse_request_line(const std::string& read_buffer, size_t& read_index,
                                             Request& request);

    static Status_Parsing parse_headers(const std::string& read_buffer, size_t& read_index,
                                        Request& request);

    static Status_Parsing parse_body(const std::string& read_buffer, size_t& read_index,
                                     Request& request);
};

#endif
