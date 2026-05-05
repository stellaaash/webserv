#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <string>

#include "Connection.hpp"
#include "Logger.hpp"
#include "Response.hpp"
#include "config.hpp"
#include "config_parser.hpp"
#include "file_manager.hpp"

void Connection::process_get_request(const FilePath& resource_path) {
    FilePath path = resource_path;

    if (is_directory(path) == true) {
        if (_request.config()->index.empty() == false) {
            // Resource to fetch becomes the index file, not the folder itself
            path = resource_path + "/" + _request.config()->index;
        } else if (_request.config()->autoindex == true) {
            _response.append_body(create_listing(path, _request.target()));
            _response.set_header("Content-Type", "text/html");
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
    if (_response.has_header("Content-Type") == false) {
        const std::string  extension = extract_extension(path);
        const std::string& mime_type = extension_to_type(extension, _config->mime_types);
        if (mime_type.empty())
            _response.set_header("Content-Type", "application/octet-stream");
        else
            _response.set_header("Content-Type", mime_type);
    }
}

void Connection::process_post_request(const FilePath& resource_path) {
    if (_request.config()->cgi.empty() == false && is_regular_file(resource_path) == true) {
        Logger(LOG_DEBUG) << "[process_post_request] - CGI called";
        _response = error_response(501, false);
    } else if (_request.config()->upload_store.empty() == false) {
        Logger(LOG_DEBUG) << "[process_post_request] - Upload called at resource_path: "
                          << resource_path;

        if (is_regular_file(resource_path) || is_directory(resource_path)) {
            // TODO Don't know if we'll do that one: 409 conflict is only returned after the client
            // already sent all of its file and the requesst was parsed, which is a waste of network
            // resources
            // In a perfect world, the parsing would say no to the request before allowing anything
            // from the body to arrive
            _response = error_response(409, false);
            return;
        }

        if (_request.is_body_spooled() == true) {
            int fd = copy_file(_request.body_path(), resource_path);
            if (fd < 0) {
                Logger(LOG_ERROR) << "[process_post_request] - copy_file: " << strerror(errno);
                _response = error_response(500, false);
                return;
            }
            close(fd);
        } else {
            int fd = create_file(resource_path);
            if (fd < 0) {
                Logger(LOG_ERROR) << "[process_post_request] - create_file: " << strerror(errno);
                _response = error_response(500, false);
                return;
            }
            if (append_file(fd, _request.body()) < 0) {
                Logger(LOG_ERROR) << "[process_post_request] - append_file: " << strerror(errno);
                _response = error_response(500, false);
                return;
            }
            close(fd);
        }
        _response.set_code(200);  // TODO return 30x See Other response
        _response.set_response_string("OK");
        _response.set_header("Content-Length", "0");
    }
}

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
        default:
            _response = error_response(501, false);
    }

    _request.set_status(REQ_PROCESSED);
}
