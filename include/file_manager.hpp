#ifndef FILEMANAGER_HPP
#define FILEMANAGER_HPP

#include <sys/types.h>

#include <map>
#include <string>

typedef std::string FilePath;

// Stores the working directory of the server
extern File_Path working_directory;

/**
 * @brief Defines the type of a FilePath, whether it is a standard file or a folder.
 */
enum Path_Type { FILE_PATH, DIR_PATH };

bool     is_regular_file(const FilePath&);
bool     is_directory(const FilePath&);
int      fetch_file(const FilePath&);
FilePath standardize_path(const std::string&);
bool     is_under_directory(const FilePath& path, const FilePath& directory);

int     create_file(const FilePath&);
ssize_t append_file(int, const std::string&);
int     remove_file(const FilePath&);

std::map<FilePath, Path_Type> get_dir_contents(const FilePath&);
std::string                   create_listing(const FilePath&);

#endif  // FILEMANAGER_HPP
