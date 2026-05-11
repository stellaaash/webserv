#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <string>

#include "Connection.hpp"
#include "Response.hpp"
#include "config.hpp"
#include "file_manager.hpp"

void Connection::process_request() {
    std::string relative_path = _request.target().substr(_request.config()->name.length());
    if (!relative_path.empty() &&  // If we still have a slash at the beginning, remove it
        relative_path[0] == '/')
        relative_path.erase(0, relative_path.find_first_not_of('/'));

    FilePath resource_path;
    switch (_request.method()) {
        case GET:
            resource_path = resolve_path(relative_path, _request.config()->root);
            process_get_request(resource_path);
            break;

        case DELETE:
            resource_path = resolve_path(relative_path, _request.config()->root);
            process_delete_request(resource_path);
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
