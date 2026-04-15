#ifndef CONNECTIONHANDLER_HPP
#define CONNECTIONHANDLER_HPP

#include <stdint.h>

#include <ctime>

#include "Connection.hpp"
#include "IHandler.hpp"
#include "config.hpp"

class ConnectionHandler : public IHandler {
public:
    ConnectionHandler(const ConfigServer* srv, int client_fd);
    virtual ~ConnectionHandler();

    virtual int      fd() const;
    virtual uint32_t interests() const;
    virtual bool     handle_event(ConnectionManager& manager, uint32_t events);
    virtual bool     is_timed_out() const;

private:
    int         _fd;
    Connection  _conn;
    std::time_t _last_activity;
    long        _timeout;

    ConnectionHandler(const ConnectionHandler&);
    ConnectionHandler& operator=(const ConnectionHandler&);
};

std::string error_response(HttpCode code);

#endif
