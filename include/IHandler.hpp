#ifndef IHANDLER_HPP
#define IHANDLER_HPP

#include <stdint.h>

class ConnectionManager;

class IHandler {
public:
    virtual ~IHandler();

    virtual int      fd() const = 0;
    virtual uint32_t interests() const = 0;
    // ConnectionManager deletes on this function returning false (epoll DEL + close + delete)
    virtual bool handle_event(ConnectionManager& manager, uint32_t events) = 0;

    virtual bool is_timed_out() const;
    virtual void timeout_connection();
};

#endif
