#include <dirent.h>

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <map>
#include <utility>

#include "FileManager.hpp"

/**
 * @brief Gets the content of a directory, and returns them as a multimpa of path to file type.
 */
std::multimap<Path_Type, File_Path> get_dir_contents(File_Path directory) {
    assert(is_directory(directory) && "File path is a directory");

    DIR* stream = opendir(directory.c_str());
    if (stream == NULL) {
        perror("[get_dir_contents] - opendir");
        throw std::exception();
    }

    struct dirent*                      result;
    std::multimap<Path_Type, File_Path> entries;

    errno = 0;
    result = readdir(stream);
    while (result != NULL) {
        Path_Type type;
        File_Path path = result->d_name;

        switch (result->d_type) {
            case DT_DIR:
                type = DIR_PATH;
                break;
            case DT_REG:
                type = FILE_PATH;
                break;
            default:
                continue;
        }

        entries.insert(std::pair<Path_Type, File_Path>(type, path));
        result = readdir(stream);
    }
    if (errno != 0) {
        perror("[get_dir_contents] - readdir");
        throw std::exception();
    }

    return entries;
}
