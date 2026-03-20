#include <fcntl.h>
#include <sys/stat.h>

#include <cstdio>
#include <cstring>

#include "config.hpp"

/**
 * @brief Checks that a file path is an actual path and not a folder or some other file type.
 */
bool is_regular_file(const File_Path& path) {
    struct stat path_stat;
    memset(&path_stat, 0, sizeof(path_stat));
    stat(path.c_str(), &path_stat);
    if (!S_ISREG(path_stat.st_mode)) {
        return false;
    }

    return true;
}

/**
 * @brief Gets a file descriptor for a give file path.
 *
 * @return The file descriptor, or -1 if an error occurred.
 */
int fetch_file(const File_Path& path) {
    int fd = open(path.c_str(), O_RDONLY);

    if (fd < 0) {
        perror("[fetch_file] - open");
        return -1;
    }

    return fd;
}
