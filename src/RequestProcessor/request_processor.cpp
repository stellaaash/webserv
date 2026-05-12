#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <string>

#include "Connection.hpp"
#include "Response.hpp"
#include "config.hpp"

void Connection::process_request() {
    std::string relative_path = _request.target().substr(_request.config()->name.length());
    if (!relative_path.empty() &&  // If we still have a slash at the beginning, remove it
        relative_path[0] == '/')
        relative_path.erase(0, relative_path.find_first_not_of('/'));

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
    if (_has_pending_cgi == true) return;
    _request.set_status(REQ_PROCESSED);
}
