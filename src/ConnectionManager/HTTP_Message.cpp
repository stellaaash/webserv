#include "HTTP_Message.hpp"

HTTP_Message::HTTP_Message() : _major_version(0), _minor_version(0), _headers(), _body() {}

HTTP_Message::HTTP_Message(const HTTP_Message& other)
    : _major_version(other._major_version),
      _minor_version(other._minor_version),
      _headers(other._headers),
      _body(other._body) {}

const HTTP_Message& HTTP_Message::operator=(const HTTP_Message& other) {
    if (this == &other) {
        return *this;
    }

    _major_version = other._major_version;
    _minor_version = other._minor_version;
    _headers = other._headers;
    _body = other._body;

    return *this;
}

HTTP_Message::~HTTP_Message() {}

const std::string& HTTP_Message::body() const {
    return _body;
}

int HTTP_Message::major_version() const {
    return _major_version;
}

int HTTP_Message::minor_version() const {
    return _minor_version;
}
/**
 * @brief Return an iterator to the first header with the name `header_name`.
 */
HTTP_Message::header_iterator HTTP_Message::header(const std::string& header_name) const {
    return _headers.lower_bound(header_name);
}

void HTTP_Message::set_version(int major_version, int minor_version) {
    _major_version = major_version;
    _minor_version = minor_version;
}

void HTTP_Message::set_header(const std::string& name, const std::string& value) {
    _headers.insert(std::pair<std::string, std::string>(name, value));
}

void HTTP_Message::append_body(const std::string& data) {
    _body.append(data);
}
