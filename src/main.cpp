#include <cerrno>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <iostream>

#include "ConnectionManager.hpp"
#include "Listener.hpp"
#include "config.hpp"
#include "socket_utils.hpp"

void clean_exit(int) {
    // TODO Clean heap-allocated stuff
    exit(0);
}

int main() {
    signal(SIGINT, clean_exit);

    Config cfg = mock_config();

    int listen_fd = make_listen_socket(cfg.server);
    if (listen_fd < 0) {
        std::perror("make_listen_socket");
        return 1;
    }

    ConnectionManager manager;
    manager.add(new Listener(&cfg.server, listen_fd));
    std::cout << "[MAIN] Server running : http://127.0.0.1:8080/" << std::endl;
    std::cout << "[MAIN] Listening on fd=" << listen_fd << std::endl;
    manager.run();

    return 0;
}
