// config.hpp
//
// @brief Defines the structs that will represent the configuration options for webserv.

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <arpa/inet.h>

#include <map>
#include <set>
#include <string>
#include <vector>

typedef unsigned int HTTP_Code;
typedef std::string  File_Path;

typedef struct Config_Location {
    std::set<HTTP_Code>              limit_except;
    std::map<HTTP_Code, std::string> redirect;  // Redirect to a URL using a specific HTTP code
    File_Path                        root;
    bool                             autoindex;
    File_Path                        index;
    File_Path                        upload_store;
    std::map<std::string, File_Path> cgi;  // Bind an extension to an interpreter
} Config_Location;

typedef struct Config_Server {
    struct sockaddr_in               listen;
    std::map<HTTP_Code, std::string> error_page;  // Error code to URI
    unsigned int                     client_max_body_size;
    std::vector<Config_Location>     location;
} Config_Server;

typedef struct Config {
    Config_Server server;
    File_Path     error_log;
} Config;

#endif
