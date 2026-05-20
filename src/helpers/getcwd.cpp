#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

// Keep track of both inode numbers and device ids
typedef std::pair<dev_t, ino_t> file_id;

file_id get_file_id(const std::string& path) {
    struct stat file_stat;

    if (stat(path.c_str(), &file_stat) == -1) {
        std::cerr << "[get_file_id] - stat:" << strerror(errno) << std::endl;
        throw std::exception();
    }

    file_id target_id;
    target_id.first = file_stat.st_dev;
    target_id.second = file_stat.st_ino;

    return target_id;
}

/**
 * @brief This function computes the working directory of the current program.
 *
 * It does this by using chdir till it reaches the root of the filesystem.
 *
 * Careful, as using chdir implies that if a single stat call fails inside this function, an
 * exception will be throw, and the program should be terminated.
 * Otherwise, the program will be left in another directory than that one it entered this
 * function in.
 */
std::string ft_getcwd() {
    std::vector<std::string> path_elements;
    std::string              path;
    const file_id            root_id = get_file_id("/");
    file_id                  current_id = get_file_id(".");

    // When the current directory is the same as the parent, we've reached the root
    while (current_id != root_id) {
        if (chdir("..") == -1) throw std::exception();

        DIR* directory_stream = opendir(".");
        if (!directory_stream) throw std::exception();
        errno = 0;  // Set to 0 to distinguish readdir errors from end of stream
        struct dirent* directory_entry = readdir(directory_stream);

        struct stat stat_results;
        stat(directory_entry->d_name, &stat_results);
        while (directory_entry != NULL) {
            if (stat_results.st_ino == current_id.second &&
                stat_results.st_dev == current_id.first) {
                // If the inode is the same, we found the last directory we were in
                path_elements.push_back(directory_entry->d_name);
                break;
            }
            directory_entry = readdir(directory_stream);
        }
        if (errno != 0) {
            closedir(directory_stream);
            throw std::exception();
        }

        closedir(directory_stream);
        current_id = get_file_id(".");
    }

    for (std::vector<std::string>::reverse_iterator element = path_elements.rbegin();
         element != path_elements.rend(); ++element) {
        path.append("/");
        path.append(*element);
    }

    std::cerr << path << std::endl;

    if (chdir(path.c_str()) == -1) throw std::exception();
    return path;
}
