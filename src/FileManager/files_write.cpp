#include <fcntl.h>

#include <cstdio>

#include "FileManager.hpp"
#include "config.hpp"

/**
 * @brief Creates a file in read-write mode, and returns its file descriptor.
 */
int create_file(const File_Path& path) {
    int fd = open(path.c_str(), O_RDWR);

    if (fd < 0) {
        perror("[create_file] - open");
        return -1;
    }

    return fd;
}
