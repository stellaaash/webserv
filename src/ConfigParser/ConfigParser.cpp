#include "ConfigParser.hpp"

#include <arpa/inet.h>
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

#include "ConfigLexer.hpp"
#include "config.hpp"

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
 * @brief Checks a path for existence of a file and access rights.
 * The directory flag specifies whether the function should check for the existence
 * of a directory, instead of a file.
 */
static int check_path(const std::string& path, bool directory) {
    assert(path.empty() == false && "String contains an actual path");

    struct stat path_stat;
    memset(&path_stat, 0, sizeof(path_stat));

    if (stat(path.c_str(), &path_stat) != 0) return 1;

    if (directory && path_stat.st_mode & S_IFDIR && access(path.c_str(), W_OK) == 0) {
        return 0;
    } else if (path_stat.st_mode & S_IFREG && access(path.c_str(), R_OK) == 0) {
        return 0;
    }

    return 1;
}

/**
 * @brief Checks a string for a the right numbers in an IP address, so nothing higher
 * than 255 or lower than 0.
 *
 * @return 0 if the numbers are good; 1 if they aren't.
 */
static int check_ip(std::string ip) {
    size_t point = 0;
    while (point < ip.npos) {
        if (std::atol(ip.c_str()) < 0 || std::atol(ip.c_str()) > 255) return 1;
        point = ip.find('.');
        ip = ip.substr(point + 1, ip.npos);
    }

    return 0;
}

/**
 * @brief Checks a configuration for duplicates in its listen directives.
 * It tries to find the same combination of address and port, or an already existing
 * 0.0.0.0 (all interfaces, which means the port is used everywhere).
 *
 * @return 0 if no dupes were found, 1 if so.
 */
static int check_listen(const Config& config) {
    const std::vector<Config_Server>&       servers = config.server;
    std::multimap<unsigned short, uint32_t> listens;

    for (std::vector<Config_Server>::const_iterator s = servers.begin(); s != servers.end(); ++s) {
        for (std::vector<struct sockaddr_in>::const_iterator l = s->listen.begin();
             l != s->listen.end(); ++l) {
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
        std::ofstream error_log(config.error_log.c_str());
        if (!error_log) throw ParserError("Invalid error_log directive");
    }

    if (check_listen(config) != 0) throw ParserError("Duplicate listen directive");

    for (std::vector<Config_Server>::const_iterator s = config.server.begin();
         s != config.server.end(); ++s) {
        for (std::map<HTTP_Code, std::string>::const_iterator e = s->error_page.begin();
             e != s->error_page.end(); ++e) {
            if (e->first <= 400 || e->first >= 599)
                throw ParserError("Invalid error_page directive (wrong HTTP code)");
        }

        for (std::map<std::string, Config_Location>::const_iterator l = s->location.begin();
             l != s->location.end(); ++l) {
            for (std::map<std::string, File_Path>::const_iterator j = l->second.cgi.begin();
                 j != l->second.cgi.end(); ++j) {
                if (check_path(j->second, false) != 0) throw ParserError("Invalid cgi directive");
            }
            if (l->second.index.empty() == false && check_path(l->second.index, false) != 0)
                throw ParserError("Invalid index directive");
            for (std::map<HTTP_Code, std::string>::const_iterator r = l->second.redirect.begin();
                 r != l->second.redirect.end(); ++r) {
                // Redirection have to use a 3XX code
                if (r->first < 300 || r->first > 399)
                    throw ParserError("Invalid redirect directive");
            }
            if (l->second.root.empty() == false && check_path(l->second.root, true) != 0)
                throw ParserError("Invalid root directive");
            if (l->second.upload_store.empty() == false &&
                check_path(l->second.upload_store, true) != 0)
                throw ParserError("Invalid upload_store directive");
        }
    }
}

/**
 * @brief Standardizes a file path.
 *
 * @description Removes a potential trailing slash.
 */
static File_Path standardize_path(const std::string& path) {
    assert(path.empty() == false && "Empty path");

    std::string standardized = path;

    if (standardized.at(standardized.size() - 1) == '/') {
        standardized.erase(standardized.size() - 1, standardized.size());
    }

    return standardized;
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
    for (std::vector<Config_Server>::const_iterator s = config.server.begin();
         s != config.server.end(); ++s) {
        if (s->listen.empty()) {
            throw ParserError("Missing listen directive in server context");
        } else if (s->location.empty()) {
            throw ParserError("Missing location directive in server context");
        } else {
            for (std::map<std::string, Config_Location>::const_iterator i = s->location.begin();
                 i != s->location.end(); ++i) {
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

Config_Server parse_server(token_iterator* t, token_iterator end) {
    Config_Server config;

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
                File_Path path = standardize_path(tokens[tokens.size() - 2].word);
                for (size_t i = 1; i < tokens.size() - 2; ++i) {
                    if (check_string(tokens[i].word, "1234567890") == false)
                        throw ParserError(tokens[1], "Wrong error_page HTTP code syntax");
                    HTTP_Code code = static_cast<unsigned int>(std::atol(tokens[i].word.c_str()));

                    config.error_page.insert(std::pair<HTTP_Code, File_Path>(code, path));
                }
            } else if (directive == "listen") {
                if (tokens.size() != 4)
                    throw ParserError(tokens[0], "Wrong number of tokens in listen directive");
                if (check_string(tokens[1].word, "1234567890.") == false)
                    throw ParserError(tokens[1], "Wrong IP syntax in listen directive");
                if (check_string(tokens[2].word, "1234567890") == false)
                    throw ParserError(tokens[2], "Wrong port syntax in listen directive");
                if (check_ip(tokens[1].word) != 0)
                    throw ParserError(tokens[1], "Wrong listen IP value");
                if (std::atol(tokens[2].word.c_str()) > 65535)
                    throw ParserError(tokens[2], "Wrong listen port value");

                struct sockaddr_in listen;
                listen.sin_family = AF_INET;
                listen.sin_port = htons(static_cast<uint16_t>(std::atol(tokens[2].word.c_str())));
                inet_pton(AF_INET, tokens[1].word.c_str(), &listen.sin_addr);

                config.listen.push_back(listen);
            } else if (directive == "location") {
                if (tokens.size() != 3)
                    throw ParserError(tokens[0], "Wrong number of tokens in location directive");
                config.location.insert(std::pair<std::string, Config_Location>(
                    tokens[1].word, parse_location((t), end)));
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

Config_Location parse_location(token_iterator* t, token_iterator end) {
    Config_Location config;

    while (*t != end) {
        std::vector<Token> tokens = parse_line(t);

        if (tokens[0].type == WORD) {
            const std::string& directive = tokens[0].word;

            if (directive == "allowed_methods") {
                if (tokens.size() < 3)
                    throw ParserError(tokens[0],
                                      "Wrong number of tokens in allowed_methods directive");
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
                config.cgi.insert(std::pair<std::string, File_Path>(
                    tokens[1].word, standardize_path(tokens[2].word)));
            } else if (directive == "index") {
                if (tokens.size() < 3)
                    throw ParserError(tokens[0], "Wrong number of tokens in index directive");
                config.index = config.root + "/" + standardize_path(tokens[1].word);
            } else if (directive == "redirect") {
                if (tokens.size() != 4)
                    throw ParserError(tokens[0], "Wrong number of tokens in redirect directive");
                if (check_string(tokens[1].word, "1234567890") == false)
                    throw ParserError(tokens[1], "Wrong redirection code syntax");
                HTTP_Code code = static_cast<HTTP_Code>(std::atol(tokens[1].word.c_str()));
                config.redirect.insert(std::pair<HTTP_Code, std::string>(code, tokens[2].word));
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
