#include "ConfigParser.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ConfigLexer.hpp"
#include "config.hpp"

ParserError::ParserError(Token& token, std::string error) : _token(token) {
    std::stringstream stream;

    stream << "Parsing error near token " << _token.word << ": " << error;

    _m_error = stream.str();
}

ParserError::~ParserError() throw() {}

const char* ParserError::what() const throw() {
    return _m_error.c_str();
}

// =============================================================================

/**
 * @brief Parse a configuration file into a standardized Config struct.
 * Calls the lexer and then the parser in succession.
 */
Config parse_file(std::ifstream& file) {
    std::vector<Token> tokens = lex_config(file);

    Config config = parse_config(tokens.begin(), tokens.end());

    // TODO check for at least a server directive (once multiple servers are possible)

    if (config.server.listen.sin_port == 0) {
        throw ParserError(*tokens.end(), "Missing listen directive in server context");
    } else if (config.server.location.empty()) {
        throw ParserError(*tokens.end(), "Missing location directive in server context");
    } else {
        for (size_t i = 0; i < config.server.location.size(); ++i) {
            if (config.server.location[i].root.empty() &&
                config.server.location[i].redirect.empty()) {
                throw ParserError(*tokens.end(), "Missing root directive in location context");
            }
        }
    }
    return config;
}

/**
 * @brief Consume all tokens until the first one that is a special character.
 * For reference, special characters are braces and semicolons.
 */
static std::vector<Token> parse_line(token_iterator* t) {
    std::vector<Token> tokens;

    while ((*t)->type == WORD) {
        tokens.push_back(**t);
        ++(*t);
    }

    // Final token should either be a brace or a semicolon
    tokens.push_back(**t);
    ++(*t);

    std::clog << "[!] - Parsed line: ";
    for (std::vector<Token>::const_iterator i = tokens.begin(); i != tokens.end(); ++i)
        std::clog << (*i).word << " ";
    std::clog << std::endl;

    return tokens;
}

/**
 * @brief Process a vector of tokens to create a Config struct out of them.
 */
Config parse_config(token_iterator t, token_iterator end) {
    Config config;

    while (t != end) {
        // TODO Take care of multiple server directives

        std::vector<Token> tokens = parse_line(&t);

        if (tokens[0].type == WORD) {
            const std::string& directive = tokens[0].word;
            if (directive == "error_log") {
                config.error_log = tokens[1].word;
            } else if (directive == "server") {
                // We pass a pointer to the iterator so the progress is replicated here
                config.server = parse_server(&t, end);
            } else {
                throw ParserError(tokens[0], "Unknown directive in root");
            }
        } else {
            throw ParserError(tokens[0], "Wrong syntax");
        }
    }

    return config;
}

Config_Server parse_server(token_iterator* t, token_iterator end) {
    Config_Server config;

    while (*t != end) {
        std::vector<Token> tokens = parse_line(t);

        if (tokens[0].type == WORD) {
            const std::string& directive = tokens[0].word;
            if (directive == "client_max_body_size") {
                // TODO Sanitization
                size_t number = static_cast<size_t>(std::atoi(tokens[1].word.c_str()));
                char   unit = tokens[1].word[tokens[1].word.size() - 1];

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
                File_Path path = tokens[tokens.size() - 2].word;
                for (size_t i = 1; i < tokens.size() - 2; ++i) {
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
                throw ParserError(tokens[0], "Unknown directive in server context");
            }
        } else if (tokens[0].type == CLOSING_BRACE) {
            break;
        } else {
            throw ParserError(tokens[0], "Wrong syntax");
        }
    }

    return config;
}

Config_Location parse_location(token_iterator* t, token_iterator end) {
    Config_Location config;

    while (*t != end) {
        std::vector<Token> tokens = parse_line(t);

        if (tokens[0].type == WORD) {
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
                        throw ParserError(tokens[i], "Unknown HTTP method");
                    }
                }
            } else if (directive == "autoindex") {
                if (tokens[1].word == "on") {
                    config.autoindex = true;
                } else if (tokens[1].word == "off") {
                    config.autoindex = false;
                } else {
                    throw ParserError(tokens[1], "Unknown value for autoindex directive");
                }
            } else if (directive == "cgi") {
                // TODO Sanitization
                config.cgi.insert(
                    std::pair<std::string, File_Path>(tokens[1].word, tokens[2].word));
            } else if (directive == "index") {
                config.index = tokens[1].word;
            } else if (directive == "redirect") {
                // TODO Sanitization
                HTTP_Code code = static_cast<HTTP_Code>(std::atoi(tokens[1].word.c_str()));
                config.redirect.insert(std::pair<HTTP_Code, std::string>(code, tokens[2].word));
            } else if (directive == "root") {
                config.root = tokens[1].word;
            } else if (directive == "upload_store") {
                config.upload_store = tokens[1].word;
            } else {
                throw ParserError(tokens[0], "Unknown directive in location context");
            }
        } else if (tokens[0].type == CLOSING_BRACE) {
            break;
        } else {
            throw ParserError(tokens[0], "Wrong syntax");
        }
    }

    return config;
}
