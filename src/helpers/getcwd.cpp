#include <unistd.h>

#include <cstdlib>
#include <string>

#include "file_manager.hpp"

std::string ft_getcwd() {
    // TODO Replace forbidden getcwd with our own function
    char* cwd = getcwd(NULL, 0);
    working_directory = cwd;
    std::free(cwd);

    return "";
}
