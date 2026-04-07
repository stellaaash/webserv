#ifndef LISTENER_HPP
#define LISTENER_HPP

#include <stdint.h>

#include "IHandler.hpp"
#include "config.hpp"

class Listener : public IHandler {
public:
    Listener(const ConfigServer* srv, int listen_fd);
    virtual ~Listener();

    virtual int      fd() const;
    virtual uint32_t interests() const;
    virtual bool     handle_event(ConnectionManager& manager, uint32_t events);

private:
    const ConfigServer* _srv;
    int                 _fd;

    Listener(const Listener&);
    Listener& operator=(const Listener&);
};

#endif
