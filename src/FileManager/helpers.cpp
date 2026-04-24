#include <cassert>
#include <string>
#include <vector>

/**
 * @brief Splits a string on a delimiter. No empty tokens are returned when delimiters follow each
 * other (for example: `split("path//to///file", '/')` would only give you `path`, `to` and `file`).
 */
std::vector<std::string> split(const std::string& s, char delimiter) {
    assert(s.empty() == false && "Empty string");

    std::vector<std::string> split_tokens;
    size_t                   index = s.find_first_not_of(delimiter);

    while (index < s.size()) {
        size_t new_index = s.find(delimiter, index);
        split_tokens.push_back(s.substr(index, new_index - index));

        index = new_index;
        while (index < s.size() && s[index] == delimiter) ++index;
    }

    return split_tokens;
}
