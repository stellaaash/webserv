#include "ConfigParser.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
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

    std::clog << "[!] - Parsed line: ";
    for (std::vector<Lexer::Token>::const_iterator i = tokens.begin(); i != tokens.end(); ++i)
        std::clog << (*i).word << " ";
    std::clog << std::endl;

    return tokens;
}

/**
 * @brief Process a vector of tokens to create a Config struct out of them.
 */
Config parse_config(Lexer::token_iterator t, Lexer::token_iterator end) {
    Config config;

    while (t != end) {
        // TODO Take care of multiple server directives

        std::vector<Lexer::Token> tokens = parse_line(&t);

        if (tokens[0].type == Lexer::WORD) {
            const std::string& directive = tokens[0].word;
            if (directive == "error_log") {
                config.error_log = t->word;
            } else if (directive == "server") {
                // We pass a pointer to the iterator so the progress is replicated here
                config.server = parse_server(&t, end);
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
        std::vector<Lexer::Token> tokens = parse_line(t);

        if (tokens[0].type == Lexer::WORD) {
            const std::string& directive = tokens[0].word;
            if (directive == "client_max_body_size") {
                // TODO Sanitization
                size_t number = static_cast<size_t>(std::atoi(tokens[1].word.c_str()));
                char   unit = tokens[1].word[tokens[1].word.size() - 2];

                switch (unit) {
                    case 'K':
                    case 'k':
                        config.client_max_body_size = number * 1024;
                        break;
                    case 'M':
                    case 'm':
                        config.client_max_body_size = number * 1024 * 1024;
                        break;
                    case 'G':
                    case 'g':
                        config.client_max_body_size = number * 1024 * 1024 * 1024;
                        break;
                    default:
                        break;
                }
            } else if (directive == "error_page") {
                File_Path path = tokens[tokens.size() - 1].word;
                for (size_t i = 0; i < tokens.size() - 2; ++i) {
                    // TODO Sanitization
                    HTTP_Code code = static_cast<unsigned int>(std::atoi(tokens[i].word.c_str()));

                    config.error_page.insert(std::pair<HTTP_Code, File_Path>(code, path));
                }
            } else if (directive == "listen") {
                // TODO Take care of multiple listen directives
                // TODO Sanitization (are the addresses and ports actually numbers?)
                // It could be possible to make a special atoi that throws on anything
                // other than positive numbers

                config.listen.sin_family = AF_INET;
                config.listen.sin_port =
                    htons(static_cast<uint16_t>(std::atoi(tokens[2].word.c_str())));
                inet_pton(AF_INET, tokens[1].word.c_str(), &config.listen.sin_addr);
            } else if (directive == "location") {
                config.location.push_back(parse_location((t), end));
            } else if (directive == "timeout") {
                // TODO Sanitization
                config.timeout = static_cast<size_t>(std::atoi(tokens[1].word.c_str()));
            } else {
                throw ParserError("Unknown directive in server context");
            }
        } else if (tokens[0].type == Lexer::CLOSING_BRACE) {
            break;
        } else {
            throw ParserError("Wrong syntax");
        }
    }

    return config;
}

Config_Location parse_location(Lexer::token_iterator* t, Lexer::token_iterator end) {
    Config_Location config;

    while (*t != end) {
        std::vector<Lexer::Token> tokens = parse_line(t);

        if (tokens[0].type == Lexer::WORD) {
            const std::string& directive = tokens[0].word;

            if (directive == "allowed_methods") {
                for (size_t i = 1; i < tokens.size() - 1; ++i) {
                    if (tokens[i].word == "GET") {
                        config.allowed_methods.insert(GET);
                    } else if (tokens[i].word == "POST") {
                        config.allowed_methods.insert(POST);
                    } else if (tokens[i].word == "DELETE") {
                        config.allowed_methods.insert(DELETE);
                    } else {
                        throw ParserError("Unknown HTTP method");
                    }
                }
            } else if (directive == "autoindex") {
                if (tokens[1].word == "on") {
                    config.autoindex = true;
                } else if (tokens[1].word == "off") {
                    config.autoindex = false;
                } else {
                    throw ParserError("Unknown value for autoindex directive");
                }
            } else if (directive == "cgi") {
                // TODO Sanitization
                config.cgi.insert(
                    std::pair<std::string, File_Path>(tokens[1].word, tokens[2].word));
            } else if (directive == "index") {
                config.index = directive;
            } else if (directive == "redirect") {
                // TODO Sanitization
                HTTP_Code code = static_cast<HTTP_Code>(std::atoi(tokens[1].word.c_str()));
                config.redirect.insert(std::pair<HTTP_Code, std::string>(code, tokens[2].word));
            } else if (directive == "root") {
                config.root = directive;
            } else if (directive == "upload_store") {
                config.upload_store = directive;
            } else {
                throw ParserError("Unknown directive in location context");
            }
        } else if (tokens[0].type == Lexer::CLOSING_BRACE) {
            break;
        } else {
            throw ParserError("Wrong syntax");
        }
    }

    return config;
}
}  // namespace Parser
}  // namespace ConfigParser

// TODO Default value if directives aren't present
