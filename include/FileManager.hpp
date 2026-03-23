#ifndef FILEMANAGER_HPP
#define FILEMANAGER_HPP

#include <sys/types.h>

#include <string>

typedef std::string File_Path;

bool      is_regular_file(const File_Path&);
bool      is_directory(const File_Path&);
int       fetch_file(const File_Path&);
File_Path standardize_path(const std::string& path);

int     create_file(const File_Path&);
ssize_t append_file(int, const std::string&);

#endif  // FILEMANAGER_HPP
