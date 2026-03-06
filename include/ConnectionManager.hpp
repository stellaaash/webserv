#ifndef CONNECTIONMANAGER_HPP
#define CONNECTIONMANAGER_HPP

#include <sys/epoll.h>

#include <map>

#include "IHandler.hpp"

class ConnectionManager {
public:
    ConnectionManager();
    ~ConnectionManager();

    void add(IHandler* h);
    void mod(IHandler* h);
    void del(IHandler* h);

    void run();

private:
    int                      _epfd;
    std::map<int, IHandler*> _handlers;

    ConnectionManager(const ConnectionManager&);
    ConnectionManager& operator=(const ConnectionManager&);
};

#endif
