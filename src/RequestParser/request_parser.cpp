#include "request_parser.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Connection.hpp"
#include "Request.hpp"

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
        return _request.status();
    }

    _request.set_query_string(extract_query_string(target));
    target = target.substr(0, target.find('?'));

    // Remove trailing slashes
    if (target != "/") {
        size_t target_end = target.find_last_not_of('/');
        target.erase(target_end + 1);
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

/**
 * @brief This function extracts the Content-Length value from a Request's headers and puts it
 * inside of the appropriate value in the same Request.
 * It sets the status to REQ_ERROR on errors.
 */
static void handle_content_length_header(Request& request, const size_t client_max_body_size) {
    assert(request.has_header("content-length"));
    size_t content_length = 0;

    if (parse_content_length_value(request.header("content-length")->second, content_length) ==
        false) {
        request.set_error_status(400);
        request.set_status(REQ_ERROR);
        return;
    }
    request.set_content_length(content_length);
    if (content_length > client_max_body_size) {
        request.set_error_status(413);
        request.set_status(REQ_ERROR);
        return;
    }
}

RequestStatus Connection::parse_headers() {
    assert(_request.status() == REQ_REQUEST_LINE);

    while (true) {
        // Find \r\n, will return npos if not found. Means it didn't finish parsing headers
        std::string::size_type line_end = _read_buffer.find("\r\n", _read_index);
        if (line_end == std::string::npos) return _request.status();

        // Empty line, end of headers.
        if (line_end == _read_index) break;
        // retrieve each line without \r\n
        std::string            line = _read_buffer.substr(_read_index, line_end - _read_index);
        std::string::size_type colon = line.find(':');
        if (colon == std::string::npos) {
            _request.set_status(REQ_ERROR);
            _request.set_error_status(400);
            return _request.status();  // Error
        }

        std::vector<std::string> parsed_values;
        std::string              key = to_lowercase(trim(line.substr(0, colon)));
        std::string              value = trim(line.substr(colon + 1));
        // Not all headers must be split
        if (key == "set-cookie")
            parsed_values.push_back(value);
        else
            parsed_values = split_header_values(value);

        if (parsed_values.empty()) {
            _request.set_header(key, "");
        } else {
            for (size_t i = 0; i < parsed_values.size(); ++i)
                _request.set_header(key, parsed_values[i]);
        }
        _read_index = line_end + 2;  // skip \r\n
    }

    // Set index after \r\n
    _read_index += 2;

    // Check for mandatory Host header, and if it is unique.
    if (!_request.has_header("host") || !is_header_unique(_request, "host")) {
        _request.set_error_status(400);
        _request.set_status(REQ_ERROR);
        return _request.status();
    }

    if (_request.has_header("content-length"))
        handle_content_length_header(_request, _config->client_max_body_size);

    if (_request.status() != REQ_ERROR) _request.set_status(REQ_HEADERS);
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
// TODO This function resolves to a location when a file's name starts with that location name
// For example, with location /upload configured, getting /upload.html resolves to /upload, but
// shouldn't To fix this, we should base ourself on whether the full location "/upload/" with the
// last / included is present in the request's target
RequestStatus Connection::resolve_location() {
    assert(_request.status() == REQ_HEADERS);

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
        if (_request.config()->allowed_methods.find(_request.method()) ==
            _request.config()->allowed_methods.end()) {
            _request.set_error_status(405);
            _request.set_status(REQ_ERROR);
        } else {
            _request.set_status(REQ_PARSED);
        }
    }
    return _request.status();
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

    if (_request.body_received() == expected) _request.set_status(REQ_PARSED);

    return _request.status();
}

RequestStatus Connection::parse_request() {
    if (_request.status() == REQ_EMPTY) parse_request_line();

    if (_request.status() == REQ_REQUEST_LINE) parse_headers();

    if (_request.status() == REQ_HEADERS) resolve_location();

    if (_request.status() == REQ_HEADERS) parse_body();

    shrink_read_buffer();
    return _request.status();
}
