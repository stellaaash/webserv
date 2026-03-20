#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <csignal>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>

#include "ConfigParser.hpp"
#include "ConnectionManager.hpp"
#include "Listener.hpp"
#include "config.hpp"
#include "socket_utils.hpp"

volatile sig_atomic_t g_stop = 0;

void clean_exit(int) {
    g_stop = 1;
}

int main(int argc, char** argv) {
    if (argc != 2) return 1;

    signal(SIGINT, clean_exit);

    // Checks if config file is a "Regular" file (non folder/pipe/etc)
    struct stat path_stat;
    memset(&path_stat, 0, sizeof(path_stat));
    stat(argv[1], &path_stat);
    if (!S_ISREG(path_stat.st_mode)) {
        std::clog << "[!] - Failed to open configuration file." << std::endl;
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

    ConnectionManager manager;
    for (ServerIter s = config.server.begin(); s != config.server.end(); ++s) {
        std::vector<int> listen_fds;
        try {
            listen_fds = make_listen_sockets(*s);
        } catch (...) {
            return 4;
        }
        for (size_t i = 0; i < listen_fds.size(); ++i) {
            manager.add(new Listener(&*s, listen_fds[i]));
        }
    }

    manager.run();

    return 0;
}
