#include "ConfigParser.hpp"

#include <cassert>
#include <iostream>
#include <vector>

#include "ConfigLexer.hpp"
#include "Request.hpp"
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
ParserError::ParserError(std::string error) : _m_error(error) {}

ParserError::~ParserError() throw() {}

const char* ParserError::what() const throw() {
    // TODO Would be really cool to store the token and line the error occurred
    // on in the exception, so that this method can print it out clearly
    return _m_error.c_str();
}

/**
 * @brief Consume all tokens until the first one that isn't a special character.
 * For reference, special characters are braces and semicolons.
 */
std::vector<Lexer::Token> parse_line(Lexer::token_iterator* t) {
    std::vector<Lexer::Token> tokens;

    while ((*t)->type == Lexer::WORD) {
        tokens.push_back(**t);
        ++(*t);
    }

    // Final token should either be a brace or a semicolon
    tokens.push_back(**t);
    ++(*t);

    return tokens;
}

/**
 * @brief Process a vector of tokens to create a Config struct out of them.
 */
Config parse_config(Lexer::token_iterator t, Lexer::token_iterator end) {
    Config config;

    while (t != end) {
        std::clog << "[!] - Processing token " << t->word << std::endl;

        // TODO Take care of multiple server directives

        std::vector<Lexer::Token> tokens = parse_line(&t);

        std::clog << "[!] - Parsed line: ";
        for (std::vector<Lexer::Token>::const_iterator i = tokens.begin(); i != tokens.end(); ++i)
            std::clog << (*i).word << " ";
        std::clog << std::endl;

        if (tokens[0].type == Lexer::WORD) {
            const std::string& directive = tokens[0].word;
            if (directive == "server") {
                // We pass a pointer to the iterator so the progress is replicated here
                config.server = parse_server(&(++t), end);
            } else if (directive == "error_log") {
                config.error_log = t->word;
            } else {
                throw ParserError("Unknown directive in root");
            }
        } else {
            throw ParserError("Wrong syntax");
        }
    }

    return config;
}

Config_Server parse_server(Lexer::token_iterator* t, Lexer::token_iterator end) {
    Config_Server config;

    while (*t != end) {
        ++(*t);
    }

    return config;
}
}  // namespace Parser
}  // namespace ConfigParser
