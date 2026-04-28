#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <exception>
#include <fstream>

#include "config.hpp"
#include "config_lexer.hpp"

class ParserError : public std::exception {
public:
    ParserError(const std::string& error);
    ParserError(const Token&, const std::string& error);
    ~ParserError() throw();

    const char* what() const throw();

private:
    Token       _token;
    std::string _m_error;
};

Config parse_file(std::ifstream&);

Config         parse_config(TokenIterator, TokenIterator end);
ConfigServer   parse_server(TokenIterator*, TokenIterator end);
ConfigLocation parse_location(TokenIterator*, TokenIterator end);

#endif
