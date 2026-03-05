// config.hpp
//
// @brief Defines the structs that will represent the configuration options for webserv.

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <arpa/inet.h>

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

typedef unsigned int HTTP_Code;
typedef std::string  File_Path;

enum HTTP_Method { UNDEFINED, GET, POST, DELETE };

typedef struct Config_Location {
    std::set<HTTP_Method>            allowed_methods;
    bool                             autoindex;
    std::map<std::string, File_Path> cgi;  // Bind an extension to an interpreter
    File_Path                        index;
    std::map<HTTP_Code, std::string> redirect;  // Redirect to a URL using a specific HTTP code
    File_Path                        root;
    File_Path                        upload_store;
} Config_Location;

typedef struct Config_Server {
    size_t                           client_max_body_size;
    std::map<HTTP_Code, std::string> error_page;  // Error code to URI
    struct sockaddr_in               listen;
    std::vector<Config_Location>     location;
    size_t                           timeout;
} Config_Server;

typedef struct Config {
    File_Path     error_log;
    Config_Server server;
} Config;

Config mock_config();

#endif
