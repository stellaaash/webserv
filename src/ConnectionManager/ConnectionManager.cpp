#include "ConnectionManager.hpp"

#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iostream>

ConnectionManager::ConnectionManager() : _epfd(epoll_create(1)) {
    if (_epfd < 0) {
        // error
    }
}

ConnectionManager::~ConnectionManager() {
    for (std::map<int, IHandler*>::iterator it = _handlers.begin(); it != _handlers.end(); ++it) {
        int       fd = it->first;
        IHandler* h = it->second;

        epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        delete h;
    }
    _handlers.clear();

    if (_epfd >= 0) close(_epfd);
}

void ConnectionManager::add(IHandler* h) {
    epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.events = h->interests();
    ev.data.ptr = h;

    // Add fd to epoll
    epoll_ctl(_epfd, EPOLL_CTL_ADD, h->fd(), &ev);
    _handlers[h->fd()] = h;
}

void ConnectionManager::mod(IHandler* h) {
    if (_handlers.find(h->fd()) == _handlers.end()) return;

    epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.events = h->interests();
    ev.data.ptr = h;

    // Switch epolls' behavior on wanted fd (EPOLLIN/OUT)
    epoll_ctl(_epfd, EPOLL_CTL_MOD, h->fd(), &ev);
}

void ConnectionManager::del(IHandler* h) {
    int                                fd = h->fd();
    std::map<int, IHandler*>::iterator it = _handlers.find(fd);
    if (it == _handlers.end()) return;

    // Remove fd from epoll
    epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL);
    _handlers.erase(it);
    std::cout << "[MANAGER] Closing fd=" << fd << std::endl;
    close(fd);
    delete h;
}

void ConnectionManager::run() {
    epoll_event events[64];  // reasonable max event size ?

    while (true) {
        int fds =
            epoll_wait(_epfd, events, 64, -1);  // returns the amount of fds ready for io operations
        std::cout << "[EPOLL] woke up with " << fds << " events" << std::endl;
        if (fds < 0) continue;

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
    }
}
