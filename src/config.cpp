#include "config.hpp"

#include <netinet/in.h>

#include <cstring>

// If nothing is provided in the location (except for the root), no default values are present.
// This would mean that you can't technically interact with the location, since no allowed
// methods have been defined. We won't set GET as default, since maybe that's not what the user
// wants, who knows.
Config_Location::Config_Location()
    : allowed_methods(), autoindex(false), cgi(), index(), redirect(), root(), upload_store() {}

Config_Server::Config_Server()
    : client_max_body_size(0), error_page(), listen(), location(), timeout(0) {}

Config::Config() : error_log(), server() {}

/**
 * @brief Create an example configuration for testing.
 *
 * @note Will be removed when parsing is functional.
 */
Config mock_config() {
    Config          conf;
    Config_Server   server;
    Config_Location loc;

    loc.allowed_methods.insert(GET);
    loc.allowed_methods.insert(POST);

    loc.redirect[301] = "index.html";
    loc.root = "./root";
    loc.index = "index.hml";
    loc.autoindex = true;

    loc.upload_store = "./root/uploads";

    loc.cgi[".rb"] = "/bin/ruby";
    loc.cgi[".py"] = "/bin/python3";

    struct sockaddr_in listen;
    listen.sin_family = AF_INET;
    listen.sin_port = htons(8080);
    listen.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server.listen.push_back(listen);

    server.error_page[404] = "/errors/404.html";
    server.error_page[500] = "/errors/500.html";
    server.error_page[413] = "/errors/413.html";

    server.timeout = 1000;
    server.client_max_body_size = 1024 * 1024;  // 1Mb

    server.location.insert(std::pair<std::string, Config_Location>("/", loc));
    conf.server = server;
    conf.error_log = "./logs/error.log";

    return (conf);
}
