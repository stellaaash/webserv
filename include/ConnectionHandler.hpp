#ifndef CONNECTIONHANDLER_HPP
#define CONNECTIONHANDLER_HPP

#include <stdint.h>

#include <ctime>

#include "CgiHandler.hpp"
#include "Connection.hpp"
#include "IHandler.hpp"
#include "config.hpp"

class ConnectionHandler : public IHandler {
public:
    ConnectionHandler(const ConfigServer* srv, int client_fd);
    virtual ~ConnectionHandler();

    virtual int      fd() const;
    virtual uint32_t interests() const;
    virtual bool     is_timed_out() const;

    void         timeout_connection();
    void         finish_cgi(const std::string& output, int cgi_status, int exit_code);
    void         clear_cgi_handler();
    virtual bool handle_event(ConnectionManager& manager, uint32_t events);

private:
    int         _fd;
    Connection  _conn;
    std::time_t _last_activity;
    long        _timeout;
    CgiHandler* _cgi_handler;

    ConnectionHandler(const ConnectionHandler&);
    ConnectionHandler& operator=(const ConnectionHandler&);
};

#endif
