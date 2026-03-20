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
#include "FileManager.hpp"
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

    if (!is_regular_file(argv[1])) {
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

    int listen_fd = make_listen_socket(config.server[0]);
    if (listen_fd < 0) {
        std::perror("make_listen_socket");
        return 4;
    }

    ConnectionManager manager;
    // FIXME: Only one listener created, regardless of the number of listeners in the configuration
    // So, we need to take into account both multiple listen directives, and multiple server ones
    manager.add(new Listener(&config.server[0], listen_fd));

    char ip_buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &config.server[0].listen[0].sin_addr, ip_buffer, INET_ADDRSTRLEN);
    std::cout << "[MAIN] Server running : http://" << ip_buffer << ":"
              << ntohs(config.server[0].listen[0].sin_port) << std::endl;
    std::cout << "[MAIN] Listening on fd=" << listen_fd << std::endl;

    manager.run();

    return 0;
}
