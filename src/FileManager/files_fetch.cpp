#include <sys/stat.h>

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
 */
int fetch_file(const File_Path& path) {
    (void)path;
    return 0;
}
