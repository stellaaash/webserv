#include "HTTP_Message.hpp"

const std::string& HTTP_Message::body() const {
    return _body;
}

/**
 * @brief Return an iterator to the first header with the name `header_name`.
 */
HTTP_Message::header_iterator HTTP_Message::header(const std::string& header_name) const {
    return _headers.lower_bound(header_name);
}

void HTTP_Message::set_header(const std::string& name, const std::string& value) {
    _headers.insert(std::pair<std::string, std::string>(name, value));
}
