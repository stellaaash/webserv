#include <cassert>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "Connection.hpp"
#include "Request.hpp"

// TODO Move helpers to separate file

static std::string trim(const std::string& s) {
    const std::string whitespace = " \t\n\r\f\v";

    size_t start = s.find_first_not_of(whitespace);
    if (start == std::string::npos) return "";

    size_t end = s.find_last_not_of(whitespace);

    return s.substr(start, end - start + 1);
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

static bool parse_content_length_value(const std::string& value, size_t& out) {
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

RequestStatus Connection::parse_request_line() {
    assert(_request.status() == REQ_EMPTY);

    std::string::size_type line_end = _read_buffer.find("\r\n", _read_index);
    if (line_end == std::string::npos) return _request.status();

    std::string line = _read_buffer.substr(_read_index, line_end - _read_index);

    // String to stream for easy parse
    std::istringstream stream(line);
    std::string        method;
    std::string        target;
    std::string        version;
    std::string        extra;

    // Fills values seperated by spaces. If extra succeeds after, Error.
    if (!(stream >> method >> target >> version) || (stream >> extra)) {
        _request.set_status(REQ_ERROR);
        _request.set_error_status(400);  // Bad request 400, too many/too few elements
        return _request.status();
    }
    if (method == "GET")
        _request.set_method(GET);
    else if (method == "POST")
        _request.set_method(POST);
    else if (method == "DELETE")
        _request.set_method(DELETE);
    else {
        _request.set_status(REQ_ERROR);
        _request.set_error_status(501);  // Not implemented
        _request.set_method(UNDEFINED);
    }

    _request.set_target(target);

    if (version != "HTTP/1.1") {
        _request.set_status(REQ_ERROR);
        _request.set_error_status(505);  // HTTP version unsupported
        return _request.status();
    }
    _request.set_version(1, 1);

    _read_index = line_end + 2;  // skip \r\n
    _request.set_status(REQ_REQUEST_LINE);

    return _request.status();
}

static bool handle_content_length_header(Request& request) {
    size_t content_length = 0;

    for (HttpMessage::HeaderIterator it = request.headers_begin(); it != request.headers_end();
         ++it) {
        if (it->first == "Content-Length") {
            if (!parse_content_length_value(it->second, content_length)) {
                request.set_error_status(400);
                request.set_status(REQ_ERROR);
                return false;
            }
        }
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

static bool is_header_unique(Request& request, const std::string& key) {
    size_t count = 0;

    for (HttpMessage::HeaderIterator it = request.headers_begin(); it != request.headers_end();
         ++it) {
        if (it->first == key) {
            ++count;
            if (count > 1) return false;
        }
    }
    return true;
}

// TODO This function really needs an overhaul. It's long and hard to read
RequestStatus Connection::parse_headers() {
    assert(_request.status() == REQ_REQUEST_LINE);

    while (true) {
        // Find \r\n, will return npos if not found. Means it didn't finish parsing headers
        std::string::size_type line_end = _read_buffer.find("\r\n", _read_index);
        if (line_end == std::string::npos) return _request.status();

        // Empty line, end of headers. Set index after \r\n
        if (line_end == _read_index) {
            _read_index += 2;

            // Check for mandatory Host header, and if it is unique.
            if (!_request.has_header("Host") || !is_header_unique(_request, "Host")) {
                _request.set_error_status(400);
                _request.set_status(REQ_ERROR);
                return _request.status();
            }
            if (!handle_content_length_header(_request)) return _request.status();
            _request.set_status(REQ_BODY);
            return _request.status();
        }
        // retrieve each line without \r\n
        std::string            line = _read_buffer.substr(_read_index, line_end - _read_index);
        std::string::size_type colon = line.find(':');
        if (colon == std::string::npos) {
            _request.set_status(REQ_ERROR);
            _request.set_error_status(400);
            return _request.status();  // Error
        }

        std::string key = trim(line.substr(0, colon));
        std::string value = trim(line.substr(colon + 1));
        // Not all headers must be split
        std::vector<std::string> values;
        if (key == "Set-Cookie")
            values.push_back(value);
        else
            values = split_header_values(value);

        if (values.empty()) {
            _request.set_header(key, "");
        } else {
            for (size_t i = 0; i < values.size(); ++i) _request.set_header(key, values[i]);
        }
        _read_index = line_end + 2;  // skip \r\n
    }
}

RequestStatus Connection::parse_body() {
    assert(_request.status() == REQ_HEADERS);

    size_t expected = _request.content_length();
    size_t received = _request.body_received();

    if (received > expected) {
        _request.set_error_status(400);
        _request.set_status(REQ_ERROR);
        return _request.status();
    }

    size_t remaining = expected - received;
    size_t available = _read_buffer.size() - _read_index;
    size_t chunk = std::min(remaining, available);

    if (chunk == 0) return _request.status();

    if (!_request.append_body_chunk(_read_buffer.data() + _read_index, chunk)) {
        _request.set_error_status(500);
        _request.set_status(REQ_ERROR);
        return _request.status();
    }

    _read_index += chunk;

    if (_request.body_received() == expected) _request.set_status(REQ_BODY);

    return _request.status();
}

/**
 * @brief Resolves which location config should be assigned to the given request, based on its
 * target.
 * It resolves in order, but the most precise location found will be set.
 * For example, if the target of the request is /images, but there are / and /images locations in
 * the configuration, the /images location will be choosen, even though / comes before
 * alphabetically.
 */
RequestStatus Connection::resolve_location() {
    assert(_request.status() == REQ_BODY);

    const std::string& request_target = _request.target();
    bool               matched = false;

    for (LocationIterator l = _config->location.begin(); l != _config->location.end(); ++l) {
        const std::string& location_name = l->first;
        const size_t       location_length = location_name.length();

        // If the target matches a location, even just as a prefix, then it's the right location
        if (request_target.substr(0, location_length) == location_name) {
            _request.set_config(&l->second);
            matched = true;
        }
    }

    if (!matched) {
        _request.set_error_status(404);
        _request.set_status(REQ_ERROR);
    } else {
        _request.set_status(REQ_PARSED);
    }
    return _request.status();
}

RequestStatus Connection::parse_request() {
    if (_request.status() == REQ_EMPTY) parse_request_line();

    if (_request.status() == REQ_REQUEST_LINE) parse_headers();

    if (_request.status() == REQ_HEADERS) parse_body();

    if (_request.status() == REQ_BODY) resolve_location();

    compact_read_buffer();
    return _request.status();
}
