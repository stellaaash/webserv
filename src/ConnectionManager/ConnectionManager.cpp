#include "ConnectionManager.hpp"

#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "signalstate.hpp"

ConnectionManager::ConnectionManager() : _epfd(epoll_create(1)) {
    if (_epfd < 0)
        std::cerr << "[ConnectionManager] epoll_create: " << strerror(errno) << std::endl;
}

ConnectionManager::~ConnectionManager() {
    for (std::map<int, IHandler*>::iterator it = _handlers.begin(); it != _handlers.end(); ++it) {
        int       fd = it->first;
        IHandler* h = it->second;

        if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL) != 0)
            std::cerr << "[~ConnectionManager] epoll_ctl: " << strerror(errno) << std::endl;
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
        std::cerr << "[ConnectionManager::add] epoll_ctl: " << strerror(errno) << std::endl;
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
        std::cerr << "[ConnectionManager::mod] epoll_ctl: " << strerror(errno) << std::endl;
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
        std::cerr << "[ConnectionManager::del] epoll_ctl: " << strerror(errno) << std::endl;
    _handlers.erase(it);
    std::cout << "[ConnectionManager] Closing fd=" << fd << std::endl;
    close(fd);
    delete h;
}

void ConnectionManager::run() {
    epoll_event events[64];  // reasonable max event size ?

    while (!g_stop) {
        // returns the amount of fds ready for io operations
        int fds = epoll_wait(_epfd, events, 64, 10000);
        if (fds < 0) {
            std::cerr << "[ConnectionManager::run] epoll_wait: " << strerror(errno) << std::endl;
            continue;
        }

        std::cout << "[EPOLL] woke up with " << fds << " events" << std::endl;

        for (int i = 0; i < fds; ++i) {
            IHandler* h = (IHandler*)events[i].data.ptr;
            std::cout << "[EPOLL] fd=" << h->fd() << " events=" << events[i].events << std::endl;
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
                std::cout << "[TIMEOUT] fd=" << h->fd() << std::endl;
                ++it;
                del(h);
            } else {
                ++it;
            }
        }
    }
}
