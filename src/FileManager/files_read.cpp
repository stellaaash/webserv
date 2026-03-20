#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstring>

#include "FileManager.hpp"

/**
 * @brief Checks that a file path is an actual path and not a folder or some other file type.
 * Also checks for read file access.
 */
bool is_regular_file(const File_Path& path) {
    assert(path.empty() == false && "String contains an actual path");

    struct stat path_stat;
    memset(&path_stat, 0, sizeof(path_stat));
    if (stat(path.c_str(), &path_stat) != 0) {
        perror("[is_regular_file] - stat");
        return -1;
    }
    if (!S_ISREG(path_stat.st_mode)) {
        return false;
    }

    int access_status = access(path.c_str(), R_OK);
    if (access_status != 0) {
        if (access_status < 0) perror("[is_regular_file] - access");
        return false;
    }

    return true;
}

/**
 * @brief Checks that a file path is a directory with write access to it.
 */
bool is_directory(const File_Path& path) {
    assert(path.empty() == false && "String contains an actual path");

    struct stat path_stat;
    memset(&path_stat, 0, sizeof(path_stat));
    stat(path.c_str(), &path_stat);
    if (stat(path.c_str(), &path_stat) != 0) {
        perror("[is_regular_file] - stat");
        return -1;
    }
    if (!S_ISDIR(path_stat.st_mode)) {
        return false;
    }

    int access_status = access(path.c_str(), W_OK);
    if (access_status != 0) {
        if (access_status < 0) perror("[is_regular_file] - access");
        return false;
    }

    return true;
}

/**
 * @brief Standardizes a file path.
 *
 * @description Removes a potential trailing slash.
 */
File_Path standardize_path(const std::string& path) {
    assert(path.empty() == false && "Empty path");

    std::string standardized = path;

    if (standardized.at(standardized.size() - 1) == '/') {
        standardized.erase(standardized.size() - 1, standardized.size());
    }

    return standardized;
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
