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

typedef struct Location_Config {
    std::set<HTTP_Code>              limit_except;
    std::map<HTTP_Code, std::string> redirect;
    File_Path                        root;
    bool                             autoindex;
    File_Path                        index;
    File_Path                        upload_store;
    std::map<std::string, File_Path> cgi;  // Bind an extension to an interpreter
} Location_Config;

typedef struct Server_Config {
    struct sockaddr_in               listen;
    std::map<HTTP_Code, std::string> error_page;  // Error code to URI
    unsigned int                     client_max_body_size;
    std::vector<Location_Config>     location;
} Server_Config;

// TODO We might get rid of that context if we don't do directive inheritance
typedef struct HTTP_Config {
    Server_Config server;
} HTTP_Config;

typedef struct Config {
    HTTP_Config http;
    File_Path   error_log;
} Config;

#endif
