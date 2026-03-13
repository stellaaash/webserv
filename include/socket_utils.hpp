#ifndef SOCKET_UTILS_HPP
#define SOCKET_UTILS_HPP

#include "config.hpp"

int set_nonblocking(int fd);
int make_listen_socket(const Config_Server& srv);

#endif
