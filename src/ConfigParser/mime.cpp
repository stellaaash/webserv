#include <strings.h>

#include <fstream>
#include <map>
#include <string>

#include "file_manager.hpp"

/**
 * @brief This function splits a line into individual tokens, delimited by whitespace.
 */
static std::vector<std::string> split_line(const std::string& line) {
    std::vector<std::string> tokens;

    size_t index_begin = 0;
    size_t chunk_size_bytes = line.find_first_of(" \t\v\r\n", index_begin) - index_begin;
    size_t index_end = index_begin + chunk_size_bytes;
    while (index_end != line.npos) {
        if (index_begin != index_end) tokens.push_back(line.substr(index_begin, chunk_size_bytes));

        index_begin = line.find_first_not_of(" \t\v\r\n", index_end);
        chunk_size_bytes = line.find_first_of(" \t\v\r\n", index_begin) - index_begin;
        index_end = index_begin + chunk_size_bytes;
    }
    tokens.push_back(line.substr(index_begin, chunk_size_bytes));

    return tokens;
}

/**
 * @brief This function parses a file as containing lines of MIME types mapped to extensions.
 *
 * @return A map from extension to its MIME types.
 */
std::map<std::string, std::string> parse_mime_types(const FilePath& path) {
    std::ifstream                      stream(path.c_str());
    std::map<std::string, std::string> types;

    while (stream.eof() == false) {
        char buffer[999];

        stream.getline(buffer, 999);
        std::vector<std::string> tokens;
        if (buffer[0] != '\0') tokens = split_line(buffer);
        if (tokens.empty()) continue;
        std::string mime_type = tokens[0];
        for (size_t i = tokens.size() - 1; i > 0; --i) {  // Go in reverse to get extensions
            std::string extension = tokens[i];
            if (extension[extension.length() - 1] == ';') extension.erase(extension.length() - 1);
            types.insert(std::pair<std::string, std::string>(extension, mime_type));
        }
    }

    return types;
}
