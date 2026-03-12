#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cerrno>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <fstream>
#include <iostream>

#include "ConfigParser.hpp"
#include "ConnectionManager.hpp"
#include "Listener.hpp"
#include "config.hpp"
#include "socket_utils.hpp"

void clean_exit(int) {
    // TODO Clean heap-allocated stuff
    exit(0);
}

int main(int argc, char** argv) {
    if (argc != 2) return 1;

    signal(SIGINT, clean_exit);

    std::ifstream config_file(argv[1]);

    Config config;
    try {
        config = parse_file(config_file);

    } catch (const ParserError& e) {
        std::cerr << "[!] - Error occurred during parsing: " << e.what() << std::endl;
        return 2;
    }

    int listen_fd = make_listen_socket(config.server);
    if (listen_fd < 0) {
        std::perror("make_listen_socket");
        return 1;
    }

    ConnectionManager manager;
    manager.add(new Listener(&config.server, listen_fd));

    char ip_buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &config.server.listen.sin_addr, ip_buffer, INET_ADDRSTRLEN);
    std::cout << "[MAIN] Server running : http://" << ip_buffer << ":"
              << ntohs(config.server.listen.sin_port) << std::endl;
    std::cout << "[MAIN] Listening on fd=" << listen_fd << std::endl;
    manager.run();
    return 0;
}
