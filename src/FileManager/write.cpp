#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <fstream>

#include "file_manager.hpp"

// Size of buffer to be used when writing data to files
const size_t write_size = 1024;

/**
 * @brief Creates a file in read-write mode, and returns its file descriptor.
 */
int create_file(const FilePath& path) {
    int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0600);

    if (fd < 0) return -1;

    return fd;
}

/**
 * @brief Appends a string's data to a file through its file descriptor.
 * Note that errors don't guarantee that the file wasn't written to.
 *
 * @return The size written, or -1 if an error occurred.
 */
ssize_t append_file(int fd, const std::string& data) {
    std::string::size_type write_index = 0;

    while (write_index < data.length()) {
        std::string::size_type chunk_size = std::min(write_size, data.length() - write_index);
        ssize_t                status = write(fd, data.c_str() + write_index, chunk_size);
        if (status < 0) {
            return -1;
        }

        write_index += static_cast<std::string::size_type>(status);
    }

    return static_cast<ssize_t>(write_index);
}

/**
 * @brief Creates a file at `destination` and copies `source`'s contents inside of it.
 *
 * @return An fd to the newly created file, or -1 if something failed.
 */
int copy_file(const FilePath& source, const FilePath& destination) {
    assert(is_regular_file(destination) == false && is_directory(destination) == false &&
           "File doesn't already exist");

    int fd = create_file(destination);
    if (fd < 0) return -1;

    std::ifstream source_data(source.c_str());
    while (source_data.eof() == false) {
        char buffer[1024];
        source_data.read(buffer, sizeof(buffer));
        if (append_file(fd, buffer) < 0) {
            return -1;
        };
    }

    return fd;
}

/**
 * @brief Removes a file and returns the status of the `remove` function called.
 */
int remove_file(const FilePath& path) {
    return remove(path.c_str());
}
