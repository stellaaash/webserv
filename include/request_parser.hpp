#ifndef REQUEST_PARSER_HPP
#define REQUEST_PARSER_HPP

#include <string>
#include <vector>

#include "Request.hpp"

// helpers.cpp

std::string              to_lowercase(const std::string&);
std::string              extract_query_string(const std::string& target);
std::string              trim(const std::string&);
std::vector<std::string> split_header_values(const std::string&);
bool                     parse_content_length_value(const std::string& value, size_t& out);
bool                     is_header_unique(Request& request, const std::string& key);

#endif  // REQUEST_PARSER_HPP
