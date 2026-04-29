#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <cstring>
#include <stack>
#include <string>
#include <vector>

#include "Logger.hpp"
#include "file_manager.hpp"

/**
 * @brief This function extracts the file extension of a given file.
 *
 * @return If the returned string is empty, then the file had no extension.
 */
std::string extract_extension(const FilePath& path) {
    size_t dot_index = path.find_last_of('.');

    if (dot_index == path.npos) return "";

    return path.substr(dot_index + 1);
}

/**
 * @brief Checks that a file path is an actual path and not a folder or some other file type.
 * Also checks for read file access.
 */
bool is_regular_file(const FilePath& path) {
    assert(path.empty() == false && "String contains an actual path");

    struct stat path_stat;
    memset(&path_stat, 0, sizeof(path_stat));
    if (stat(path.c_str(), &path_stat) != 0) {
        Logger(LOG_ERROR) << "[is_regular_file] stat: " << strerror(errno);
        return false;
    }
    if (!S_ISREG(path_stat.st_mode)) {
        return false;
    }

    if (access(path.c_str(), R_OK) != 0) {
        Logger(LOG_ERROR) << "[is_regular_file] access: " << strerror(errno);
        return false;
    }

    return true;
}

/**
 * @brief Checks that a file path is a directory with write access to it.
 */
bool is_directory(const FilePath& path) {
    assert(path.empty() == false && "String contains an actual path");

    struct stat path_stat;
    memset(&path_stat, 0, sizeof(path_stat));
    if (stat(path.c_str(), &path_stat) != 0) {
        Logger(LOG_ERROR) << "[is_directory] stat: " << strerror(errno);
        return false;
    }
    if (!S_ISDIR(path_stat.st_mode)) {
        return false;
    }

    if (access(path.c_str(), W_OK) != 0) {
        Logger(LOG_ERROR) << "[is_directory] access: " << strerror(errno);
        return false;
    }

    return true;
}

/**
 * @brief Returns the length of a file on disk.
 */
size_t file_length(const FilePath& path) {
    struct stat file_stat;

    if (stat(path.c_str(), &file_stat) == -1)
        Logger(LOG_ERROR) << "[file_length] stat: " << strerror(errno);

    Logger(LOG_DEBUG) << path << " is of size " << file_stat.st_size;
    return static_cast<size_t>(file_stat.st_size);
}

/**
 * @brief Gets a file descriptor for a give file path.
 *
 * @return The file descriptor, or -1 if an error occurred.
 */
int fetch_file(const FilePath& path) {
    int fd = open(path.c_str(), O_RDONLY);

    if (fd < 0) {
        Logger(LOG_ERROR) << "[fetch_file] open: " << strerror(errno);
        return -1;
    }

    return fd;
}

/**
 * @brief Standardizes a file path.
 *
 * @description Removes a potential trailing slash.
 */
FilePath standardize_path(const std::string& path) {
    assert(path.empty() == false && "Empty path");

    // First split on /
    std::vector<std::string> split_tokens = split(path, '/');

    std::stack<std::string> s;
    for (size_t index = 0; index < split_tokens.size(); ++index) {
        if (split_tokens[index] == ".")
            ;
        else if (split_tokens[index] == "..") {
            if (s.empty() == false) s.pop();
        } else {
            s.push(split_tokens[index]);
        }
    }

    // Finally concatenate into a full, absolute path
    FilePath standardized;
    while (s.empty() == false) {
        std::string token = s.top();
        token.insert(0, "/");
        standardized.insert(0, token);

        s.pop();
    }

    if (path[0] != '/') standardized.insert(0, working_directory);
    return standardized;
}
