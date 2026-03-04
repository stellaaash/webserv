#include "ConfigParser.hpp"

#include <iostream>
#include <vector>

#include "ConfigLexer.hpp"
#include "config.hpp"

namespace ConfigParser {
/**
 * @brief Parse a configuration file into a standardized Config struct.
 */
Config parse_file(std::ifstream& file) {
    std::vector<Lexer::Token> tokens = Lexer::lex_config(file);

    return Parser::parse_config(tokens.begin(), tokens.end());
}

namespace Parser {
/**
 * @brief Process a vector of tokens to create a Config struct out of them.
 */
Config parse_config(Lexer::token_iterator t, Lexer::token_iterator end) {
    Config config;

    while (t != end) {
        std::clog << "[!] - Processing token " << t->word << std::endl;
        ++t;
    }

    return config;
}
}  // namespace Parser
}  // namespace ConfigParser
