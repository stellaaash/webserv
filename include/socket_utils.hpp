#ifndef SOCKET_UTILS_HPP
#define SOCKET_UTILS_HPP

#include "config.hpp"

int              set_nonblocking(int fd);
std::vector<int> make_listen_sockets(const Config_Server&);

#endif
