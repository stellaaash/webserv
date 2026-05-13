#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "Request.hpp"

/**
 * @brief Minimal implementation to fetch the query string from the target.
 * /over/there?name=ferret
 */
std::string extract_query_string(const std::string& target) {
    size_t pos = target.find('?');

    if (pos == std::string::npos) return "";

    return target.substr(pos + 1);
}

/**
 * @brief Removes preceeding and trailing whitespace from a string.
 */
std::string trim(const std::string& s) {
    const std::string whitespace = " \t\n\r\f\v";

    size_t start = s.find_first_not_of(whitespace);
    if (start == std::string::npos) return "";

    size_t end = s.find_last_not_of(whitespace);

    return s.substr(start, end - start + 1);
}

/**
 * @brief When receiving HTTP headers with multiple values separated by commas, this function will
 * split those values into individual strings to append to the headers multimap of the Request
 * object.
 */
std::vector<std::string> split_header_values(const std::string& value) {
    std::vector<std::string> result;
    size_t                   start = 0;

    while (start < value.size()) {
        size_t comma = value.find(',', start);

        // push final value
        if (comma == std::string::npos) {
            std::string end = trim(value.substr(start));
            if (!end.empty()) result.push_back(end);
            break;
        }

        // push trimmed value
        std::string trimmed = trim(value.substr(start, comma - start));
        if (!trimmed.empty()) result.push_back(trimmed);

        start = comma + 1;
    }

    return result;
}

bool parse_chunk_size(const std::string& str, size_t& out) {
    if (str.empty()) return false;

    size_t value = 0;

    for (size_t i = 0; i < str.size(); ++i) {
        int digit;

        if (str[i] >= '0' && str[i] <= '9')
            digit = str[i] - '0';
        else if (str[i] >= 'a' && str[i] <= 'f')
            digit = str[i] - 'a' + 10;
        else if (str[i] >= 'A' && str[i] <= 'F')
            digit = str[i] - 'A' + 10;
        else
            return false;

        value = value * 16 + static_cast<size_t>(digit);
    }

    out = value;
    return true;
}

bool parse_content_length_value(const std::string& value, size_t& out) {
    if (value.empty()) return false;

    for (size_t i = 0; i < value.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(value[i]))) return false;
    }

    std::istringstream stream(value);
    unsigned long      parsed = 0;
    stream >> parsed;
    if (!stream || !stream.eof()) return false;
    if (parsed > std::numeric_limits<size_t>::max()) return false;

    out = static_cast<size_t>(parsed);
    return true;
}

bool is_header_unique(Request& request, const std::string& key) {
    return request.header_count(key) == 1;
}
