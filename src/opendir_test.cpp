#include <dirent.h>
#include <sys/types.h>

#include <cstdio>
#include <iostream>

/**
 * This demo opens the directory the executable is in, and lists all of its contents
 * to stdout.
 */
int main(void) {
    // Open the src directory
    DIR* dir_fd = opendir(".");

    if (!dir_fd) {
        perror("opendir");
        return 1;
    }

    while (true) {
        struct dirent* entry = readdir(dir_fd);

        if (!entry) {
            perror("readdir");
            break;
        }

        std::cout << entry->d_name << std::endl;
    }
}
