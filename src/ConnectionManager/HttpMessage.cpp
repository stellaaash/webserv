#include "HttpMessage.hpp"

HttpMessage::HttpMessage() : _major_version(0), _minor_version(0), _headers(), _body() {}

HttpMessage::HttpMessage(const HttpMessage& other)
    : _major_version(other._major_version),
      _minor_version(other._minor_version),
      _headers(other._headers),
      _body(other._body) {}

const HttpMessage& HttpMessage::operator=(const HttpMessage& other) {
    if (this == &other) {
        return *this;
    }

    _major_version = other._major_version;
    _minor_version = other._minor_version;
    _headers = other._headers;
    _body = other._body;

    return *this;
}

HttpMessage::~HttpMessage() {}

const std::string& HttpMessage::body() const {
    return _body;
}

int HttpMessage::major_version() const {
    return _major_version;
}

int HttpMessage::minor_version() const {
    return _minor_version;
}

/**
 * @brief Return an iterator to the first header with the name `header_name`.
 */
HttpMessage::HeaderIterator HttpMessage::header(const std::string& header_name) const {
    return _headers.lower_bound(header_name);
}

void HttpMessage::set_version(int major_version, int minor_version) {
    _major_version = major_version;
    _minor_version = minor_version;
}

void HttpMessage::set_header(const std::string& name, const std::string& value) {
    _headers.insert(std::pair<std::string, std::string>(name, value));
}

void HttpMessage::append_body(const std::string& data) {
    _body.append(data);
}

bool HttpMessage::has_header(const std::string& header_name) const {
    return _headers.find(header_name) != _headers.end();
}

HttpMessage::HeaderIterator HttpMessage::headers_begin() const {
    return _headers.begin();
}

HttpMessage::HeaderIterator HttpMessage::headers_end() const {
    return _headers.end();
}
