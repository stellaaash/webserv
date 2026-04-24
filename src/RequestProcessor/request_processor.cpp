#include <cerrno>
#include <cstring>

#include "Connection.hpp"
#include "Logger.hpp"
#include "Response.hpp"
#include "config.hpp"
#include "file_manager.hpp"

ResponseStatus Connection::process_get_request(const FilePath& resource_path) {
    // Fetch the resource or generate content
    if (is_directory(resource_path) == true) {
        if (_request.config().autoindex == true) {
            _response.append_body(create_listing(resource_path));
            _response.set_code(200);
            _response.set_response_string("OK");
        } else {
            _response = error_response(404);
        }
        return _response.status();
    } else if (!is_regular_file(resource_path)) {
        _response = error_response(404);
        return _response.status();
    }

    int fd = fetch_file(resource_path);
    if (fd < 0) {
        Logger(LOG_ERROR) << "[process_request] - fetch_file" << strerror(errno);
        if (errno == EACCES)
            _response = error_response(403);
        else if (errno == ENOENT)
            _response = error_response(404);
        else if (errno == EINVAL || errno == ENAMETOOLONG) {
            _response = error_response(400);
        } else
            _response = error_response(500);
        return _response.status();
    }

    // _response.set_fd(fd);
    _response.append_body("Hewwo :3");  // TODO Temporary, need to implement reading from fds
    _response.set_code(200);
    _response.set_response_string("OK");
    std::stringstream stream;
    if (_response.body().empty() == false) {
        stream << _response.body().size();
    } else if (_response.fd() >= 0) {
        stream << file_length(resource_path);
    } else {
        stream << 0;  // If no body is present and no file is to be read, the body is empty
    }
    _response.set_header("Content-Length", stream.str());
    // TODO Set headers, MIME types??

    return _response.status();
}

ResponseStatus Connection::process_request() {
    // TODO Isolate the resource path we need (need the fileman pr merged first)
    // This is hardcoded for now until I can get the real resource path
    FilePath resource_path = "html/index.html";

    switch (_request.method()) {
        case GET:
            process_get_request(resource_path);
            break;
        case POST:
        case DELETE:
        default:
            _response = error_response(501);
    }

    _request.set_status(REQ_PROCESSED);
    return _response.status();
}
