#include "Listener.hpp"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

#include "ConnectionHandler.hpp"
#include "ConnectionManager.hpp"
#include "socket_utils.hpp"

Listener::Listener(const ConfigServer* srv, int listen_fd) : _srv(srv), _fd(listen_fd) {}
Listener::~Listener() {}

int Listener::fd() const {
    return _fd;
}
uint32_t Listener::interests() const {
    return EPOLLIN;
}

bool Listener::handle_event(ConnectionManager& manager, uint32_t events) {
    if (events & (EPOLLERR | EPOLLHUP)) {
        std::cerr << "[LISTENER " << _fd << "] Error: Wrong epoll event";
        return false;
    }

    if (!(events & EPOLLIN))  // If not a read event, leave
        return true;

    while (true) {
        int client_fd = accept(_fd, NULL, NULL);
        if (client_fd < 0) break;  // nothing more to accept

        if (set_nonblocking(client_fd) < 0) {
            close(client_fd);
            continue;
        }

        std::cout << "[LISTENER] New client accepted fd=" << client_fd << std::endl;

        manager.add(new ConnectionHandler(_srv, client_fd));
    }
    return true;  // keeps listener
}
