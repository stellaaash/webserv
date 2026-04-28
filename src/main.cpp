#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

#include "ConnectionManager.hpp"
#include "Listener.hpp"
#include "Logger.hpp"
#include "config.hpp"
#include "config_parser.hpp"
#include "file_manager.hpp"
#include "socket_utils.hpp"

volatile sig_atomic_t g_stop = 0;

FilePath working_directory;

void clean_exit(int) {
    g_stop = 1;
}

int main(int argc, char** argv) {
    if (argc != 2) return 1;

    signal(SIGINT, clean_exit);

    // TODO Replace forbidden getcwd with our own function
    char* cwd = getcwd(NULL, 0);
    working_directory = cwd;
    free(cwd);

    if (!is_regular_file(argv[1])) {
        std::cerr << "[!] - Failed to open configuration file." << std::endl;
        return 2;
    }

    std::ifstream config_file(argv[1]);

    Config config;
    try {
        config = parse_file(config_file);
    } catch (const ParserError& e) {
        std::cerr << "[!] - " << e.what() << std::endl;
        return 3;
    }
    config_file.close();

    logger_init(config.error_log);

    ConnectionManager manager;
    for (ServerIterator s = config.server.begin(); s != config.server.end(); ++s) {
        const ConfigServer& server_config = *s;
        std::vector<int>    listen_fds;
        try {
            listen_fds = make_listen_sockets(server_config);
        } catch (...) {
            return 4;
        }
        for (size_t i = 0; i < listen_fds.size(); ++i) {
            manager.add(new Listener(&server_config, listen_fds[i]));
        }
    }

    manager.run();

    return 0;
}
