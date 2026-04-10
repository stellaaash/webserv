#include "Listener.hpp"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ConnectionHandler.hpp"
#include "ConnectionManager.hpp"
#include "Logger.hpp"
#include "socket_utils.hpp"

Listener::Listener(const ConfigServer* srv, int listen_fd) : _srv(srv), _fd(listen_fd) {}

Listener::~Listener() {}

int Listener::fd() const {
    return _fd;
}

/**
 * @brief Returns the interests of a Listener instance, which are always incoming connections and
 * nothing else.
 */
uint32_t Listener::interests() const {
    return EPOLLIN;
}

/**
 * @brief Handles events for a listener, accepting all incoming connection attempts.
 */
bool Listener::handle_event(ConnectionManager& manager, uint32_t events) {
    if (events & (EPOLLERR | EPOLLHUP)) {
        Logger(LOG_ERROR) << "[LISTENER " << _fd << "] Error: Wrong epoll event";
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

        Logger(LOG_GENERAL) << "[LISTENER] New client accepted fd=" << client_fd;

        manager.add(new ConnectionHandler(_srv, client_fd));
    }
    return true;  // keeps listener
}
