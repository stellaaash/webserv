#ifndef CONNHANDLER_HPP
#define CONNHANDLER_HPP

#include <stdint.h>

#include <ctime>

#include "Connection.hpp"
#include "IHandler.hpp"
#include "config.hpp"

class ConnHandler : public IHandler {
public:
    ConnHandler(const Config_Server* srv, int client_fd);
    virtual ~ConnHandler();

    virtual int      fd() const;
    virtual uint32_t interests() const;
    virtual bool     handle_event(ConnectionManager& manager, uint32_t events);
    virtual bool     is_timed_out() const;

private:
    int         _fd;
    Connection  _conn;
    std::time_t _last_activity;
    long        _timeout;

    ConnHandler(const ConnHandler&);
    ConnHandler& operator=(const ConnHandler&);
};

#endif
