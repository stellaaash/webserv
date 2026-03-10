#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <exception>
#include <fstream>
#include <vector>

#include "ConfigLexer.hpp"
#include "config.hpp"
Config parse_file(std::ifstream&);

class ParserError : public std::exception {
public:
    ParserError(Token&, std::string error);
    ~ParserError() throw();

    const char* what() const throw();

private:
    Token       _token;
    std::string _m_error;
};

std::vector<Token> parse_line(token_iterator*);

Config          parse_config(token_iterator, token_iterator end);
Config_Server   parse_server(token_iterator*, token_iterator end);
Config_Location parse_location(token_iterator*, token_iterator end);

#endif
