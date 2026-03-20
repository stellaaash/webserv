#ifndef FILEMANAGER_HPP
#define FILEMANAGER_HPP

#include <string>

typedef std::string File_Path;

bool      is_regular_file(const File_Path&);
bool      is_directory(const File_Path&);
int       fetch_file(const File_Path&);
File_Path standardize_path(const std::string& path);

int create_file(const File_Path&);

#endif  // FILEMANAGER_HPP
