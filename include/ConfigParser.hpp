#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <exception>
#include <fstream>
#include <vector>

#include "ConfigLexer.hpp"
#include "config.hpp"
namespace ConfigParser {
Config parse_file(std::ifstream&);

namespace Parser {
class ParserError : public std::exception {
public:
    ParserError(Lexer::Token&, std::string error);
    ~ParserError() throw();

    const char* what() const throw();

private:
    Lexer::Token _token;
    std::string  _m_error;
};

std::vector<Lexer::Token> parse_line(Lexer::token_iterator*);

Config          parse_config(Lexer::token_iterator, Lexer::token_iterator end);
Config_Server   parse_server(Lexer::token_iterator*, Lexer::token_iterator end);
Config_Location parse_location(Lexer::token_iterator*, Lexer::token_iterator end);
}  // namespace Parser
}  // namespace ConfigParser

#endif
