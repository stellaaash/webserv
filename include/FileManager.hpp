#ifndef FILEMANAGER_HPP
#define FILEMANAGER_HPP

#include "config.hpp"

bool is_regular_file(const File_Path& path);
int  fetch_file(const File_Path&);

#endif  // FILEMANAGER_HPP
