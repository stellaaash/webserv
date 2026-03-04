#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <exception>
#include <fstream>

#include "ConfigLexer.hpp"
#include "config.hpp"
namespace ConfigParser {
Config parse_file(std::ifstream&);

namespace Parser {
class ParserError : public std::exception {};

Config          parse_config(Lexer::token_iterator, Lexer::token_iterator end);
Config_Server   parse_server(Lexer::token_iterator, Lexer::token_iterator end);
Config_Location parse_location(Lexer::token_iterator, Lexer::token_iterator end);
}  // namespace Parser
}  // namespace ConfigParser

#endif
