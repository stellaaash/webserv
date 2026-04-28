#include <dirent.h>

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <map>
#include <sstream>
#include <utility>

#include "file_manager.hpp"

/**
 * @brief Gets the content of a directory, and returns them as a multimap of path to file type.
 */
std::map<FilePath, Path_Type> get_dir_contents(const FilePath& directory) {
    assert(is_directory(directory) && "File path is a directory");

    DIR* stream = opendir(directory.c_str());
    if (stream == NULL) {
        perror("[get_dir_contents] - opendir");
        throw std::exception();
    }

    struct dirent*                result;
    std::map<FilePath, Path_Type> entries;

    // Set to 0 in order to know if readdir returning NULL is an error or just EOF
    errno = 0;
    result = readdir(stream);
    while (result != NULL) {
        FilePath  path = result->d_name;
        Path_Type type;

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

        entries.insert(std::pair<FilePath, Path_Type>(path, type));
        result = readdir(stream);
    }
    closedir(stream);
    if (errno != 0) {
        perror("[get_dir_contents] - readdir");
        throw std::exception();
    }

    return entries;
}

/**
 * @brief Creates an HTML listing of a directory, containing links to all entries inside.
 */
std::string create_listing(const FilePath& directory, const std::string& target) {
    std::map<FilePath, Path_Type> entries = get_dir_contents(directory);
    std::stringstream             html_listing;

    html_listing << "<html>\n";
    html_listing << "<head>\n";
    html_listing << "<title>" << target << "</title>\n";
    html_listing << "</head>\n";
    html_listing << "<body>\n";
    html_listing << "<h1>Listing of " << target << "</h1>\n";
    html_listing << "<ul>\n";
    for (std::map<FilePath, Path_Type>::const_iterator e = entries.begin(); e != entries.end();
         ++e) {
        std::string path = e->first;
        if (e->second == DIR_PATH) path.append("/");

        if (e->first == ".")
            html_listing << "<li><a href=\"" << target << "\">" << path << "</a></li>\n";
        else
            html_listing << "<li><a href=\"" << target << "/" << e->first << "\">" << path
                         << "</a></li>\n";
    }
    html_listing << "</ul>\n";
    html_listing << "</body>\n";
    html_listing << "</html>\n";

    return html_listing.str();
}
