#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <stack>
#include <string>
#include <vector>

#include "file_manager.hpp"

/**
 * @brief Checks that a file path is an actual path and not a folder or some other file type.
 * Also checks for read file access.
 */
bool is_regular_file(const FilePath& path) {
    assert(path.empty() == false && "String contains an actual path");

    struct stat path_stat;
    memset(&path_stat, 0, sizeof(path_stat));
    if (stat(path.c_str(), &path_stat) != 0) {
        perror("[is_regular_file] - stat");
        return false;
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
bool is_directory(const FilePath& path) {
    assert(path.empty() == false && "String contains an actual path");

    struct stat path_stat;
    memset(&path_stat, 0, sizeof(path_stat));
    if (stat(path.c_str(), &path_stat) != 0) {
        perror("[is_regular_file] - stat");
        return false;
    }
    if (!S_ISDIR(path_stat.st_mode)) {
        return false;
    }

    int access_status = access(path.c_str(), W_OK);
    if (access_status != 0) {
        if (access_status < 0) perror("[is_directory] - access");
        return false;
    }

    return true;
}

/**
 * @brief Standardizes a file path.
 *
 * @description Removes a potential trailing slash, as well as processing .. or . tokens.
 */
FilePath standardize_path(const std::string& path) {
    assert(path.empty() == false && "Empty path");

    std::stack<std::string> tokens;
    size_t                  begin = 0;
    for (size_t i = 0; i < path.size(); ++i) {
        std::string folder_name;
        if (path[i] == '/') {  // End of the folder name
            folder_name = path.substr(begin, i - begin);
            begin = i + 1;
        } else if (i == path.size() - 1) {  // End of the path
            folder_name = path.substr(begin, i - begin + 1);
            begin = i + 1;
        }

        if (folder_name == ".")
            ;
        else if (folder_name == "..")
            tokens.pop();
        else if (!folder_name.empty())
            tokens.push(folder_name);
    }

    std::vector<std::string> in_order_tokens;
    while (!tokens.empty()) {
        in_order_tokens.insert(in_order_tokens.begin(), tokens.top());
        tokens.pop();
    }

    File_Path standardized;
    if (path[0] != '/')  // If path is relative
        standardized = working_directory;
    for (std::vector<std::string>::const_iterator i = in_order_tokens.begin();
         i != in_order_tokens.end(); ++i) {
        if (i->empty()) continue;
        standardized.append("/");
        standardized.append(*i);
    }

    return standardized;
}

// TODO: function to check if a standardized path is higher than a root

/**
 * @brief Gets a file descriptor for a give file path.
 *
 * @return The file descriptor, or -1 if an error occurred.
 */
int fetch_file(const FilePath& path) {
    int fd = open(path.c_str(), O_RDONLY);

    if (fd < 0) {
        perror("[fetch_file] - open");
        return -1;
    }

    return fd;
}
