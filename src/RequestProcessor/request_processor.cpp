#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <string>

#include "Connection.hpp"
#include "Response.hpp"
#include "config.hpp"

void Connection::process_request() {
    // FIXME GETting /imagescard_normal.png still works
    std::string relative_path = _request.target().substr(_request.config()->name.length());

    // TODO Redirection checking should probably happen before the body arrives
    if (_request.config()->redirect.first >= 300 && _request.config()->redirect.first <= 399) {
        _response.set_code(301);
        _response.set_response_string("Moved Permanently");
        _response.set_header("Content-Type", "text/html");
        _response.set_header("Content-Length", "0");

        const std::string& location_name = _request.config()->redirect.second;
        if (location_name == "/")
            _response.set_header("Location", location_name + relative_path);
        else
            _response.set_header("Location", location_name + "/" + relative_path);
        _request.set_status(REQ_PROCESSED);
        return;
    }

    switch (_request.method()) {
        case GET:
            process_get_request(relative_path);
            break;

        case DELETE:
            process_delete_request(relative_path);
            break;

        case POST:
            process_post_request(relative_path);
            break;

        default:
            _response = error_response(501, false);
            break;
    }
    _request.set_status(REQ_PROCESSED);
}
