#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "ConfigParser.hpp"
#include "Logger.hpp"
#include "config.hpp"
#include "config_lexer.hpp"
#include "file_manager.hpp"

// We construct the error string in the constructors to be able to return a pointer to it later
// Having the stringstream be part of the what() member function wouldn't work because of
// having to return a pointer to an object local to the function
ParserError::ParserError(const std::string& error) : _token() {
    std::stringstream stream;

    stream << "Parsing error: " << error;

    _m_error = stream.str();
}

ParserError::ParserError(const Token& token, const std::string& error) : _token(token) {
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
 * @brief Checks a configuration for duplicates in its listen directives.
 * It tries to find the same combination of address and port, or an already existing
 * 0.0.0.0 (all interfaces, which means the port is used everywhere).
 *
 * @return 0 if no dupes were found, 1 if so.
 */
static int check_listen(const Config& config) {
    const std::vector<ConfigServer>&        servers = config.server;
    std::multimap<unsigned short, uint32_t> listens;

    for (ServerIterator s = servers.begin(); s != servers.end(); ++s) {
        for (ListenIterator l = s->listen.begin(); l != s->listen.end(); ++l) {
            const unsigned short port = l->sin_port;
            const uint32_t       address = l->sin_addr.s_addr;

            // Check for duplicate entries
            std::multimap<unsigned short, uint32_t>::const_iterator i = listens.find(port);
            // If there's already a listen directive with that port, and the current one has address
            // 0.0.0.0, then it's a dupe
            if (i != listens.end() && address == 0) return 1;
            while (i != listens.end() && i->first == port) {
                // If there was already a 0.0.0.0 with that port, it has to be a dupe
                if (i->second == 0) return 1;
                // If there was the same address with the same port, it's also a dupe
                if (i->second == address) return 1;

                ++i;
            }

            listens.insert(listens.end(), std::pair<unsigned short, uint32_t>(port, address));
        }
    }

    return 0;
}

static void check_config(const Config& config) {
    if (config.error_log.empty() == false) {
        if (is_directory(config.error_log) == false)
            throw ParserError("Invalid error_log directive, not a directory");
    }

    if (check_listen(config) != 0) throw ParserError("Duplicate listen directive");

    for (ServerIterator s = config.server.begin(); s != config.server.end(); ++s) {
        for (ErrorPageIterator e = s->error_page.begin(); e != s->error_page.end(); ++e) {
            if (e->first <= 400 || e->first >= 599)
                throw ParserError("Invalid error_page directive (wrong HTTP code)");
        }

        for (LocationIterator l = s->location.begin(); l != s->location.end(); ++l) {
            if (l->second.allowed_methods.empty())
                Logger(LOG_ERROR) << "[PARSING] - Location " << l->first
                                  << " is missing an allowed method.";
            for (CgiIterator j = l->second.cgi.begin(); j != l->second.cgi.end(); ++j) {
                if (is_regular_file(j->second) == false) throw ParserError("Invalid cgi directive");
            }
            if (l->second.index.empty() == false && is_regular_file(l->second.index) == false)
                throw ParserError("Invalid index directive");
            for (ErrorPageIterator r = l->second.redirect.begin(); r != l->second.redirect.end();
                 ++r) {
                // Redirection have to use a 3XX code
                if (r->first < 300 || r->first > 399)
                    throw ParserError("Invalid redirect directive");
            }
            if (l->second.root.empty() == false && is_directory(l->second.root) == false)
                throw ParserError("Invalid root directive");
            if (l->second.upload_store.empty() == false &&
                is_directory(l->second.upload_store) == false)
                throw ParserError("Invalid upload_store directive");
        }
    }
}

/**
 * @brief Parse a configuration file into a standardized Config struct.
 * Calls the lexer and then the parser in succession, and checks for semantic correctness.
 */
Config parse_file(std::ifstream& file) {
    const std::vector<Token> tokens = lex_config(file);

    const Config config = parse_config(tokens.begin(), tokens.end());

    if (config.server.empty()) {
        throw ParserError("Missing server directive");
    }
    for (ServerIterator s = config.server.begin(); s != config.server.end(); ++s) {
        if (s->listen.empty()) {
            throw ParserError("Missing listen directive in server context");
        } else if (s->location.empty()) {
            throw ParserError("Missing location directive in server context");
        } else {
            for (LocationIterator i = s->location.begin(); i != s->location.end(); ++i) {
                if (i->second.root.empty() && i->second.redirect.empty()) {
                    throw ParserError("Missing root directive in location context");
                }
            }
        }
    }

    check_config(config);

    return config;
}

/*
 * @brief Checks that a string contains only the characters held within the allowed_set.
 *
 * @return false if the string contains erroneous characters, true if it doesn't.
 */
static bool check_string(const std::string& string, const std::string& allowed_set) {
    for (size_t i = 0; i < string.size(); ++i)
        if (allowed_set.find_first_of(string[i]) == string.npos) return false;

    return true;
}

/**
 * @brief Consume all tokens until the first one that is a special character.
 * For reference, special characters are braces and semicolons.
 */
static std::vector<Token> parse_line(TokenIterator* t) {
    std::vector<Token> tokens;

    while ((*t)->type == WORD) {
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
Config parse_config(TokenIterator t, TokenIterator end) {
    Config config;

    while (t != end) {
        std::vector<Token> tokens = parse_line(&t);

        if (tokens[0].type == WORD) {
            const std::string& directive = tokens[0].word;
            if (directive == "error_log") {
                if (tokens.size() != 3)
                    throw ParserError(tokens[0], "Wrong number of tokens in error_log directive");
                config.error_log = standardize_path(tokens[1].word);
            } else if (directive == "server") {
                if (tokens.size() != 2)
                    throw ParserError(tokens[0], "Wrong number of tokens in server directive");
                // We pass a pointer to the iterator so the progress is replicated here
                config.server.push_back(parse_server(&t, end));
            } else {
                throw ParserError(tokens[0], "Unknown directive in root");
            }
        } else {
            throw ParserError(tokens[0], "Wrong syntax");
        }
    }

    return config;
}

ConfigServer parse_server(TokenIterator* t, TokenIterator end) {
    ConfigServer config;

    while (*t != end) {
        std::vector<Token> tokens = parse_line(t);

        if (tokens[0].type == WORD) {
            const std::string& directive = tokens[0].word;
            if (directive == "client_max_body_size") {
                if (tokens.size() != 3)
                    throw ParserError(tokens[0],
                                      "Wrong number of tokens in client_max_body_size directive");
                if (check_string(tokens[1].word, "1234567890kKmMgG") == false)
                    throw ParserError(tokens[1], "Wrong client_max_body_size syntax");
                if (tokens[1].word.length() > 5)
                    throw ParserError(tokens[1],
                                      "Wrong client_max_body_size value - Hint: set to 0 to "
                                      "disable, or use units like M or G to set higher values");

                size_t number = static_cast<size_t>(std::atol(tokens[1].word.c_str()));
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
                        throw ParserError(tokens[1], "Wrong or missing unit for bytes");
                        break;
                }
            } else if (directive == "error_page") {
                if (tokens.size() < 4)
                    throw ParserError(tokens[0], "Wrong number of tokens in error_page directive");
                FilePath path = standardize_path(tokens[tokens.size() - 2].word);
                for (size_t i = 1; i < tokens.size() - 2; ++i) {
                    if (check_string(tokens[i].word, "1234567890") == false)
                        throw ParserError(tokens[1], "Wrong error_page HTTP code syntax");
                    HttpCode code = static_cast<unsigned int>(std::atol(tokens[i].word.c_str()));

                    config.error_page.insert(std::pair<HttpCode, FilePath>(code, path));
                }
            } else if (directive == "listen") {
                if (tokens.size() != 4)
                    throw ParserError(tokens[0], "Wrong number of tokens in listen directive");
                if (check_string(tokens[1].word, "1234567890.") == false)
                    throw ParserError(tokens[1], "Wrong IP syntax in listen directive");
                if (check_string(tokens[2].word, "1234567890") == false)
                    throw ParserError(tokens[2], "Wrong port syntax in listen directive");
                if (std::atol(tokens[2].word.c_str()) > 65535)
                    throw ParserError(tokens[2], "Wrong listen port value");

                struct addrinfo  hints;
                struct addrinfo* results;

                memset(&hints, 0, sizeof(hints));
                hints.ai_family = AF_INET;
                hints.ai_socktype = SOCK_STREAM;
                hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;  // Only accept numeric hosts

                int gai_status =
                    getaddrinfo(tokens[1].word.c_str(), tokens[2].word.c_str(), &hints, &results);
                if (gai_status != 0) throw ParserError(tokens[1], gai_strerror(gai_status));

                config.listen.push_back(*reinterpret_cast<struct sockaddr_in*>(results->ai_addr));

                freeaddrinfo(results);
            } else if (directive == "location") {
                if (tokens.size() != 3)
                    throw ParserError(tokens[0], "Wrong number of tokens in location directive");
                ConfigLocation location = parse_location(t, end);
                location.name = tokens[1].word;  // Have the name in the struct directly
                config.location.insert(
                    std::pair<std::string, ConfigLocation>(tokens[1].word, location));
            } else if (directive == "timeout") {
                if (tokens.size() != 3)
                    throw ParserError(tokens[0], "Wrong number of tokens in timeout directive");
                if (check_string(tokens[1].word, "1234567890") == false)
                    throw ParserError(tokens[1], "Wrong timeout syntax");
                config.timeout = static_cast<size_t>(std::atol(tokens[1].word.c_str()));
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

ConfigLocation parse_location(TokenIterator* t, TokenIterator end) {
    ConfigLocation config;

    while (*t != end) {
        std::vector<Token> tokens = parse_line(t);

        if (tokens[0].type == WORD) {
            const std::string& directive = tokens[0].word;

            if (directive == "allowed_methods") {
                if (tokens.size() < 3)
                    throw ParserError(tokens[0],
                                      "Wrong number of tokens in allowed_methods directive");
                for (size_t i = 1; i < tokens.size() - 1; ++i) {
                    HttpMethod m;
                    if (tokens[i].word == "GET") {
                        m = GET;
                    } else if (tokens[i].word == "POST") {
                        m = POST;
                    } else if (tokens[i].word == "DELETE") {
                        m = DELETE;
                    } else {
                        throw ParserError(tokens[i], "Unknown HTTP method");
                    }
                    if (config.allowed_methods.find(m) != config.allowed_methods.end())
                        throw ParserError(tokens[i], "Duplicate allowed method");
                    config.allowed_methods.insert(m);
                }
            } else if (directive == "autoindex") {
                if (tokens.size() != 3)
                    throw ParserError(tokens[0], "Wrong number of tokens in autoindex directive");
                if (tokens[1].word == "on") {
                    config.autoindex = true;
                } else if (tokens[1].word == "off") {
                    config.autoindex = false;
                } else {
                    throw ParserError(tokens[1], "Unknown value for autoindex directive");
                }
            } else if (directive == "cgi") {
                if (tokens.size() != 4)
                    throw ParserError(tokens[0], "Wrong number of tokens in cgi directive");
                config.cgi.insert(std::pair<std::string, FilePath>(
                    tokens[1].word, standardize_path(tokens[2].word)));
            } else if (directive == "index") {
                if (tokens.size() < 3)
                    throw ParserError(tokens[0], "Wrong number of tokens in index directive");
                config.index = config.root + "/" + tokens[1].word;
            } else if (directive == "redirect") {
                if (tokens.size() != 4)
                    throw ParserError(tokens[0], "Wrong number of tokens in redirect directive");
                if (check_string(tokens[1].word, "1234567890") == false)
                    throw ParserError(tokens[1], "Wrong redirection code syntax");
                HttpCode code = static_cast<HttpCode>(std::atol(tokens[1].word.c_str()));
                config.redirect.insert(std::pair<HttpCode, std::string>(code, tokens[2].word));
            } else if (directive == "root") {
                if (tokens.size() != 3)
                    throw ParserError(tokens[0], "Wrong number of tokens in root directive");
                config.root = standardize_path(tokens[1].word);
            } else if (directive == "upload_store") {
                if (tokens.size() != 3)
                    throw ParserError(tokens[0],
                                      "Wrong number of tokens in upload_store directive");
                config.upload_store = standardize_path(tokens[1].word);
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
