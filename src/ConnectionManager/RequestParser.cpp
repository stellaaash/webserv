#include "RequestParser.hpp"

#include <sstream>
#include <string>
#include <vector>

#include "Request.hpp"

static std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t')) ++start;

    size_t end = s.size();
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) --end;

    return s.substr(start, end - start);
}

static std::vector<std::string> split_header_values(const std::string& value) {
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

Status_Parsing RequestParser::parse(std::string& read_buffer, size_t& read_index,
                                    Request& request) {
    if (request.status() == EMPTY) parse_request_line(read_buffer, read_index, request);

    if (request.status() == REQUEST_LINE) parse_headers(read_buffer, read_index, request);

    if (request.status() == BODY) parse_body(read_buffer, read_index, request);

    return request.status();
}

Status_Parsing RequestParser::parse_request_line(const std::string& read_buffer, size_t& read_index,
                                                 Request& request) {
    if (request.status() != EMPTY) return request.status();

    std::string::size_type line_end = read_buffer.find("\r\n", read_index);
    if (line_end == std::string::npos) return request.status();

    std::string line = read_buffer.substr(read_index, line_end - read_index);

    // String to stream for easy parse
    std::istringstream stream(line);
    std::string        method;
    std::string        target;
    std::string        version;
    std::string        extra;

    // Fills values seperated by spaces. If extra succeeds after, Error.
    if (!(stream >> method >> target >> version) || (stream >> extra)) {
        request.set_status(ERROR);
        request.set_error_status(400);  // Bad request 400, too many/too few elements
        return request.status();
    }
    if (method == "GET")
        request.set_method(GET);
    else if (method == "POST")
        request.set_method(POST);
    else if (method == "DELETE")
        request.set_method(DELETE);
    else {
        request.set_status(ERROR);
        request.set_error_status(501);  // Not implemented
        request.set_method(UNDEFINED);
    }

    request.set_target(target);

    if (version != "HTTP/1.1") {
        request.set_status(ERROR);
        request.set_error_status(505);  // HTTP version unsupported
        return request.status();
    }
    request.set_version(1, 1);

    read_index = line_end + 2;  // skip \r\n
    request.set_status(REQUEST_LINE);

    return request.status();
}

Status_Parsing RequestParser::parse_headers(const std::string& read_buffer, size_t& read_index,
                                            Request& request) {
    if (request.status() != REQUEST_LINE) return request.status();

    while (true) {
        // Find \r\n, will return npos if not found. Means it didn't finish parsing headers
        std::string::size_type line_end = read_buffer.find("\r\n", read_index);
        if (line_end == std::string::npos) return request.status();

        // Empty line, end of headers. Set index after \r\n
        if (line_end == read_index) {
            read_index += 2;

            // if Headers include "Content-Length", set parse status as body
            if (request.has_header("Content-Length")) {
                HTTP_Message::header_iterator it = request.header("Content-Length");
                std::istringstream            stream(it->second);
                size_t                        len = 0;
                if (!(stream >> len)) return request.status();  // TODO : Exception/Error on NaN

                request.set_content_length(len);

                if (len > 0) {
                    request.set_status(BODY);
                    return request.status();
                }
            }

            request.set_status(PARSED);
            return request.status();
        }
        // retrieve each line without \r\n
        std::string            line = read_buffer.substr(read_index, line_end - read_index);
        std::string::size_type colon = line.find(':');
        if (colon == std::string::npos) {
            request.set_status(ERROR);
            request.set_error_status(400);
            return request.status();  // Error
        }

        std::string key = trim(line.substr(0, colon));
        std::string value = trim(line.substr(colon + 1));
        // Not all headers must be split. Set-cookies shouldn't
        std::vector<std::string> values = split_header_values(value);

        if (values.empty()) {
            request.set_header(key, "");
        } else {
            for (size_t i = 0; i < values.size(); ++i) request.set_header(key, values[i]);
        }
        read_index = line_end + 2;  // skip \r\n
    }
}

Status_Parsing RequestParser::parse_body(const std::string& read_buffer, size_t& read_index,
                                         Request& request) {
    if (request.status() != BODY) return request.status();

    size_t expected = request.content_length();
    size_t available = read_buffer.size() - read_index;

    if (available < expected) return request.status();

    std::string body = read_buffer.substr(read_index, expected);
    request.append_body(body);

    read_index += expected;
    request.set_status(PARSED);

    return request.status();
}
