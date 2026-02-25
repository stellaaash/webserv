#ifndef HTTP_MESSAGE_HPP
#define HTTP_MESSAGE_HPP

#include <map>
#include <string>

class HTTP_Message {
public:
    // What the header getter will return, this makes it easier to iterate over duplicate headers
    typedef std::multimap<std::string, std::string>::const_iterator header_iterator;

    int                major_version() const;
    int                minor_version() const;
    header_iterator    header(const std::string&) const;
    const std::string& body() const;

    void set_version(int major_version, int minor_version);
    void set_header(const std::string&, const std::string&);
    void append_body(const std::string&);

private:
    int                                     _major_version;
    int                                     _minor_version;
    std::multimap<std::string, std::string> _headers;
    std::string                             _body;
};

#endif
