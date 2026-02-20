#include "config.hpp"

#include <cstring>

Config mock_config() {
    Config          conf;
    Config_Server   server;
    Config_Location loc;

    loc.limit_except.insert(405);  // Not allowed placeholder
    loc.limit_except.insert(501);  // Not implemented placeholder

    loc.redirect[301] = "index.html";
    loc.root = "./root";
    loc.index = "index.hml";
    loc.autoindex = true;

    loc.upload_store = "./root/uploads";

    loc.cgi[".rb"] = "/bin/ruby";
    loc.cgi[".py"] = "/bin/python3";

    std::memset(&server.listen, 0, sizeof(server.listen));
    server.listen.sin_family = AF_INET;
    server.listen.sin_port = htons(8080);
    server.listen.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    server.error_page[404] = "/errors/404.html";
    server.error_page[500] = "/errors/500.html";
    server.error_page[413] = "/errors/413.html";

    server.timeout = 1000;
    server.client_max_body_size = 1024 * 1024;  // 1Mb

    server.location.push_back(loc);
    conf.server = server;
    conf.error_log = "./logs/error.log";

    return (conf);
}
