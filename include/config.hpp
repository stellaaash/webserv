// config.hpp
//
// @brief Defines the structs that will represent the configuration options for webserv.

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "file_manager.hpp"

typedef unsigned int HttpCode;

enum HttpMethod { UNDEFINED, GET, POST, DELETE };

typedef struct ConfigLocation {
    ConfigLocation();

    std::string name;

    std::set<HttpMethod>            allowed_methods;
    bool                            autoindex;
    std::map<std::string, FilePath> cgi;  // Bind an extension to an interpreter
    // File to serve when requesting a directory (relative from the requested file)
    std::string                     index;
    std::map<HttpCode, std::string> redirect;  // Redirect to a URL using a specific HTTP code
    FilePath                        root;
    FilePath                        upload_store;
} ConfigLocation;

typedef struct ConfigServer {
    ConfigServer();

    // Parsed directly from mime.types - extension to possible MIME type
    std::map<std::string, std::string>    mime_types;
    size_t                                client_max_body_size;
    std::map<HttpCode, std::string>       error_page;  // Error code to URI
    std::vector<struct sockaddr_in>       listen;
    std::map<std::string, ConfigLocation> location;
    size_t                                timeout;
} ConfigServer;

typedef struct Config {
    Config();

    // An empty error_log path means everything is logged to stderr
    FilePath                  error_log;
    std::vector<ConfigServer> server;
} Config;

typedef std::vector<ConfigServer>::const_iterator             ServerIterator;
typedef std::vector<struct sockaddr_in>::const_iterator       ListenIterator;
typedef std::map<HttpCode, std::string>::const_iterator       ErrorPageIterator;
typedef std::map<std::string, ConfigLocation>::const_iterator LocationIterator;
typedef std::map<std::string, FilePath>::const_iterator       CgiIterator;

#endif
