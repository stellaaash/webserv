#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "Request.hpp"

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

bool handle_content_length_header(Request& request) {
    size_t content_length = 0;

    if (request.has_header("Content-Length") &&
        !parse_content_length_value(request.header("Content-Length")->second, content_length)) {
        request.set_error_status(400);
        request.set_status(REQ_ERROR);
        return false;
    }
    request.set_content_length(content_length);

    if (content_length > request.client_max_body_size()) {
        request.set_error_status(413);
        request.set_status(REQ_ERROR);
        return false;
    }
    if (content_length > 0) {
        request.set_status(REQ_HEADERS);
        return false;
    }
    return true;
}

bool is_header_unique(Request& request, const std::string& key) {
    return request.header_count(key) == 1;
}
