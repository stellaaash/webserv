#ifndef FILEMANAGER_HPP
#define FILEMANAGER_HPP

#include "config.hpp"

bool is_regular_file(const File_Path&);
int  fetch_file(const File_Path&);

int create_file(const File_Path&);

#endif  // FILEMANAGER_HPP
