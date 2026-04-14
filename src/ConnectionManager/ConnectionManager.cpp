#include "ConnectionManager.hpp"

#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include "Logger.hpp"
#include "signal_state.hpp"

ConnectionManager::ConnectionManager() : _epfd(epoll_create(1)) {
    if (_epfd < 0) Logger(LOG_ERROR) << "[ConnectionManager] epoll_create: " << strerror(errno);
}

ConnectionManager::~ConnectionManager() {
    for (std::map<int, IHandler*>::iterator it = _handlers.begin(); it != _handlers.end(); ++it) {
        int       fd = it->first;
        IHandler* h = it->second;

        if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL) != 0)
            Logger(LOG_ERROR) << "[~ConnectionManager] epoll_ctl: " << strerror(errno);
        close(fd);
        delete h;
    }
    _handlers.clear();

    if (_epfd >= 0) close(_epfd);
}

int ConnectionManager::add(IHandler* h) {
    epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = h->interests();
    ev.data.ptr = h;

    // Add fd to epoll
    if (epoll_ctl(_epfd, EPOLL_CTL_ADD, h->fd(), &ev) != 0) {
        Logger(LOG_ERROR) << "[ConnectionManager::add] epoll_ctl: " << strerror(errno);
        return 1;
    };
    _handlers[h->fd()] = h;

    return 0;
}

int ConnectionManager::mod(IHandler* h) {
    if (_handlers.find(h->fd()) == _handlers.end()) return 1;

    epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.events = h->interests();
    ev.data.ptr = h;

    // Switch epolls' behavior on wanted fd (EPOLLIN/OUT)
    if (epoll_ctl(_epfd, EPOLL_CTL_MOD, h->fd(), &ev) != 0) {
        Logger(LOG_ERROR) << "[ConnectionManager::mod] epoll_ctl: " << strerror(errno);
        return 1;
    }
    return 0;
}

void ConnectionManager::del(IHandler* h) {
    int                                fd = h->fd();
    std::map<int, IHandler*>::iterator it = _handlers.find(fd);
    if (it == _handlers.end()) return;

    // Remove fd from epoll
    if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL) != 0)
        Logger(LOG_ERROR) << "[ConnectionManager::del] epoll_ctl: " << strerror(errno);
    _handlers.erase(it);
    Logger(LOG_GENERAL) << "[ConnectionManager] Closing fd=" << fd;
    close(fd);
    delete h;
}

void ConnectionManager::run() {
    epoll_event events[64];  // reasonable max event size ?

    while (!g_stop) {
        // returns the amount of fds ready for io operations
        int fds = epoll_wait(_epfd, events, 64, 10000);
        if (fds < 0) {
            Logger(LOG_ERROR) << "[ConnectionManager::run] epoll_wait: " << strerror(errno);
            continue;
        }

        for (int i = 0; i < fds; ++i) {
            IHandler* h = static_cast<IHandler*>(events[i].data.ptr);
            Logger(LOG_GENERAL) << "[EPOLL] fd=" << h->fd() << " events=" << events[i].events;
            // Keep or delete connection
            bool keep = h->handle_event(*this, events[i].events);
            if (!keep) {
                del(h);
            } else {
                // if interests() changed. IN/OUT
                mod(h);
            }
        }
        for (std::map<int, IHandler*>::iterator it = _handlers.begin(); it != _handlers.end();) {
            IHandler* h = it->second;

            if (h->is_timed_out()) {
                Logger(LOG_GENERAL) << "[TIMEOUT] fd=" << h->fd();
                ++it;
                del(h);
            } else {
                ++it;
            }
        }
    }
}
