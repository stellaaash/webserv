#ifndef FILEMANAGER_HPP
#define FILEMANAGER_HPP

#include <sys/types.h>

#include <map>
#include <string>

typedef std::string File_Path;

/**
 * @brief Defines the type of a File_Path, whether it is a standard file or a folder.
 */
enum Path_Type { FILE_PATH, DIR_PATH };

// files_read.cpp
bool      is_regular_file(const File_Path&);
bool      is_directory(const File_Path&);
int       fetch_file(const File_Path&);
File_Path standardize_path(const std::string& path);

// files_write.cpp
int     create_file(const File_Path&);
ssize_t append_file(int, const std::string&);

// dir_listing.cpp
std::map<File_Path, Path_Type> get_dir_contents(File_Path);

#endif  // FILEMANAGER_HPP
