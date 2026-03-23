#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>

#include "FileManager.hpp"

// Size of buffer to be used when writing data to files
const size_t write_size = 1024;

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
