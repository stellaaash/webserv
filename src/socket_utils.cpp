#include "socket_utils.hpp"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <exception>
#include <iostream>
#include <vector>

#include "config.hpp"

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("set_nonblocking (fcntl)");
        return -1;
    }
    return 0;
}

/**
 * @brief Creates a vector of listening sockets based on all listen directives in a server context.
 */
std::vector<int> make_listen_sockets(const Config_Server& config) {
    std::vector<int> fds;

    for (ListenIter l = config.listen.begin(); l != config.listen.end(); ++l) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) throw;

        int yes = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        if (bind(fd, reinterpret_cast<const struct sockaddr*>(&*l), sizeof(*l)) < 0) {
            perror("make_listen_sockets (bind)");
            for (size_t i = 0; i < fds.size(); ++i) close(fds[i]);
            throw std::exception();
        }
        if (listen(fd, 128) < 0) {
            perror("make_listen_sockets (listen)");
            for (size_t i = 0; i < fds.size(); ++i) close(fds[i]);
            throw std::exception();
        }
        if (set_nonblocking(fd) < 0) {
            for (size_t i = 0; i < fds.size(); ++i) close(fds[i]);
            throw std::exception();
        }

        char ip_buffer[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &l->sin_addr, ip_buffer, INET_ADDRSTRLEN);
        std::clog << "[make_listen_sockets] - Created fd " << fd << " to listen at " << ip_buffer
                  << " on port " << ntohs(l->sin_port) << std::endl;

        fds.push_back(fd);
    }
    return fds;
}
