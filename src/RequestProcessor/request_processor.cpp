#include <cerrno>
#include <cstring>

#include "Connection.hpp"
#include "Logger.hpp"
#include "Response.hpp"
#include "config.hpp"
#include "file_manager.hpp"

ResponseStatus Connection::process_get_request(const FilePath& resource_path) {
    FilePath path = resource_path;

    if (is_directory(path) == true) {
        if (_request.config()->index.empty() == false) {
            // Resource to fetch becomes the index file, not the folder itself
            path = resource_path + "/" + _request.config()->index;
        } else if (_request.config()->autoindex == true) {
            _response.append_body(create_listing(path));
            _response.set_code(200);
            _response.set_response_string("OK");
        } else {
            _response = error_response(404, false);
        }
    } else if (!is_regular_file(path)) {  // We don't serve anything other than files or directories
        _response = error_response(404, false);
    }

    // Fetch file on disk
    if (_response.is_error() == false && _response.body().empty() == true) {
        int fd = fetch_file(path);
        if (fd < 0) {
            Logger(LOG_ERROR) << "[process_request] - fetch_file" << strerror(errno);
            if (errno == EACCES)
                _response = error_response(403, false);
            else if (errno == ENOENT)
                _response = error_response(404, false);
            else if (errno == EINVAL || errno == ENAMETOOLONG) {
                _response = error_response(400, false);
            } else
                _response = error_response(500, false);
        } else {
            _response.set_fd(fd);
            _response.set_code(200);
            _response.set_response_string("OK");
        }
    }

    std::stringstream stream;
    if (_response.body().empty() == false) {
        stream << _response.body().size();
    } else if (_response.fd() >= 0) {
        stream << file_length(path);
    } else {
        Logger(LOG_ERROR) << "[process_get_request] - Impossible state! Response has no body and "
                             "no fd to read from";
        stream << 0;
    }
    _response.set_header("Content-Length", stream.str());
    // TODO Set headers, MIME types??

    return _response.status();
}

ResponseStatus Connection::process_request() {
    std::string relative_path = _request.target().substr(_request.config()->name.length());
    if (relative_path[0] == '/')  // If we still have a slash at the beginning, remove it
        relative_path.erase(0, relative_path.find_first_not_of('/'));
    FilePath resource_path = resolve_path(relative_path, _request.config()->root);

    switch (_request.method()) {
        case GET:
            process_get_request(resource_path);
            break;
        case POST:
        case DELETE:
        default:
            _response = error_response(501, false);
    }

    _request.set_status(REQ_PROCESSED);
    return _response.status();
}
