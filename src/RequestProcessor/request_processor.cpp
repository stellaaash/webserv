#include <cerrno>
#include <cstring>

#include "Connection.hpp"
#include "Logger.hpp"
#include "Response.hpp"
#include "config.hpp"

ResponseStatus Connection::process_get_request(const FilePath& resource_path) {
    // Fetch the resource or generate content
    if (is_directory(resource_path) == true) {
        if (_request.config().autoindex == true) {
            _response.append_body(create_listing(_request.config().root));
            _response.set_code(200);
            _response.set_response_string("OK");
            std::stringstream stream;
            stream << _response.body().size();
            _response.set_header("Content-Length", stream.str());
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

    _response.set_fd(fd);
    _response.set_code(200);
    _response.set_response_string("OK");
    // TODO Temporary -- will be replaced with fd reading and body sending
    _response.set_header("Content-Length", "7");
    _response.append_body("Hello\r\n");
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
