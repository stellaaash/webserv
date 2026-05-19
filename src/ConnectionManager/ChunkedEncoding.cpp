#include "Connection.hpp"

#include <sys/socket.h>
#include <sys/types.h>

#include <cassert>
#include <cerrno>
#include <cstring>

#include "Request.hpp"
#include <request_parser.hpp>

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

bool Connection::handle_chunked_encoding_header(Request& request) {
    _is_chunked = false;
    _chunk_size = 0;
    _chunk_received = 0;

    bool has_transfer_encoding = false;
    bool has_chunked = false;

    for (HttpMessage::HeaderIterator it = request.headers_begin(); it != request.headers_end();
         ++it) {
        if (it->first != "transfer-encoding") continue;

        has_transfer_encoding = true;

        if (trim(it->second) == "chunked") has_chunked = true;
    }

    if (!has_transfer_encoding) return true;

    if (!has_chunked) {
        request.set_error_status(400);
        request.set_status(REQ_ERROR);
        return false;
    }

    if (request.has_header("content-length")) {
        request.set_error_status(400);
        request.set_status(REQ_ERROR);
        return false;
    }

    _is_chunked = true;
    _chunk_size = 0;
    _chunk_received = 0;

    request.set_status(REQ_HEADERS);
    return false;
}

RequestStatus Connection::parse_chunked_body() {
    while (_read_index < _read_buffer.size()) {
        if (_chunk_size == 0 && _chunk_received == 0) {
            size_t line_end = _read_buffer.find("\r\n", _read_index);  // get line from read buffer
            if (line_end == std::string::npos) return _request.status();

            std::string line =
                _read_buffer.substr(_read_index, line_end - _read_index);  // Extract size line

            size_t semi = line.find(';');
            if (semi != std::string::npos) line = line.substr(0, semi);  // cut chunk extension

            if (!parse_chunk_size(trim(line), _chunk_size)) {  // trim spaces
                _request.set_error_status(400);
                _request.set_status(REQ_ERROR);
                return _request.status();
            }

            _read_index = line_end + 2;  // jump past \r\n

            if (_chunk_size == 0) {  // End of chunked request
                if (_read_buffer.size() - _read_index < 2)
                    return _request.status();  // in case read buffer doesn't reach the end

                if (_read_buffer[_read_index] != '\r' || _read_buffer[_read_index + 1] != '\n') {
                    _request.set_error_status(400);
                    _request.set_status(REQ_ERROR);
                    return _request.status();
                }

                _read_index += 2;
                _request.set_content_length(_request.body_received());
                _request.set_status(REQ_PARSED);  // Chunked request parsed
                return _request.status();
            }
        }

        size_t remaining = _chunk_size - _chunk_received;
        size_t available = _read_buffer.size() - _read_index;
        size_t chunk = std::min(remaining, available);

        if (chunk == 0) return _request.status();

        if (!_request.append_body_chunk(_read_buffer.data() + _read_index, chunk)) {
            _request.set_error_status(500);
            _request.set_status(REQ_ERROR);
            return _request.status();
        }

        _read_index += chunk;
        _chunk_received += chunk;

        if (_chunk_received == _chunk_size) {
            if (_read_buffer.size() - _read_index < 2) return _request.status();

            if (_read_buffer[_read_index] != '\r' || _read_buffer[_read_index + 1] != '\n') {
                _request.set_error_status(400);
                _request.set_status(REQ_ERROR);
                return _request.status();
            }

            _read_index += 2;
            _chunk_size = 0;
            _chunk_received = 0;
        }
    }

    return _request.status();
}