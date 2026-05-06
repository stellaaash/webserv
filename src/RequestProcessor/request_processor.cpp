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
    if (relative_path[0] == '/')  // If we still have a slash at the beginning, remove it
        relative_path.erase(0, relative_path.find_first_not_of('/'));

    FilePath resource_path;
    if (_request.config()->upload_store.empty() == false) {
        resource_path = resolve_path(relative_path, _request.config()->upload_store);
    } else {
        resource_path = resolve_path(relative_path, _request.config()->root);
    }

    switch (_request.method()) {
        case GET:
            process_get_request(resource_path);
            break;
        case POST:
            process_post_request(resource_path);
            break;
        case DELETE:
            process_delete_request(resource_path);
            break;
        default:
            _response = error_response(501, false);
    }

    _request.set_status(REQ_PROCESSED);
}
