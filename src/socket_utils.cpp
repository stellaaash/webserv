#include "socket_utils.hpp"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) return -1;
    return 0;
}

int make_listen_socket(const Config_Server& srv) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    if (bind(fd, (const struct sockaddr*)&srv.listen[0], sizeof(srv.listen)) < 0) {
        close(fd);
        return -1;
    }
    if (listen(fd, 128) < 0) {
        close(fd);
        return -1;
    }
    if (set_nonblocking(fd) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}
