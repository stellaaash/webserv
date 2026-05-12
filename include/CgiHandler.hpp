#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <sys/types.h>

#include <ctime>
#include <string>

#include "IHandler.hpp"

class ConnectionHandler;
class ConnectionManager;

class CgiHandler : public IHandler {
public:
    CgiHandler(pid_t pid, int stdout_fd, int stderr_fd, long timeout, ConnectionHandler* client);

    virtual ~CgiHandler();

    virtual int      fd() const;
    virtual uint32_t interests() const;
    virtual bool     is_timed_out() const;
    virtual void     timeout_connection();
    virtual bool     handle_event(ConnectionManager& manager, uint32_t events);

    ConnectionHandler* client() const;
    void               detach_client();
    void               abort_cgi();

private:
    pid_t              _pid;
    int                _stdout_fd;
    int                _stderr_fd;
    long               _timeout;
    std::time_t        _start;
    std::string        _output;
    std::string        _error;
    ConnectionHandler* _client;
    bool               _stdout_eof;
    bool               _stderr_eof;

    void read_stdout();
    void read_stderr();
};

#endif
