#ifndef HTTP_MESSAGE_HPP
#define HTTP_MESSAGE_HPP

#include <map>
#include <string>

class HTTP_Message {
public:
    // What the header getter will return, this makes it easier to iterate over duplicate headers
    typedef std::multimap<std::string, std::string>::const_iterator header_iterator;

    virtual ~HTTP_Message();

    virtual const std::string& body() const;
    virtual header_iterator    header(const std::string&) const;

    virtual void set_header(const std::string&, const std::string&);

private:
    int                                     _major_version;
    int                                     _minor_version;
    std::multimap<std::string, std::string> _headers;
    std::string                             _body;
};

#endif
