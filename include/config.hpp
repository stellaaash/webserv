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

#include "FileManager.hpp"

typedef unsigned int HTTP_Code;

enum HTTP_Method { UNDEFINED, GET, POST, DELETE };

typedef struct Config_Location {
    Config_Location();

    std::set<HTTP_Method>            allowed_methods;
    bool                             autoindex;
    std::map<std::string, File_Path> cgi;  // Bind an extension to an interpreter
    File_Path                        index;
    std::map<HTTP_Code, std::string> redirect;  // Redirect to a URL using a specific HTTP code
    File_Path                        root;
    File_Path                        upload_store;
} Config_Location;

typedef struct Config_Server {
    Config_Server();

    size_t                                 client_max_body_size;
    std::map<HTTP_Code, std::string>       error_page;  // Error code to URI
    std::vector<struct sockaddr_in>        listen;
    std::map<std::string, Config_Location> location;
    size_t                                 timeout;
} Config_Server;

typedef struct Config {
    Config();

    // An empty error_log path means everything is logged to stderr
    File_Path                  error_log;
    std::vector<Config_Server> server;
} Config;

typedef std::vector<Config_Server>::const_iterator             ServerIter;
typedef std::vector<struct sockaddr_in>::const_iterator        ListenIter;
typedef std::map<HTTP_Code, std::string>::const_iterator       ErrorPageIter;
typedef std::map<std::string, Config_Location>::const_iterator LocationIter;
typedef std::map<std::string, File_Path>::const_iterator       CgiIter;

Config mock_config();

#endif
