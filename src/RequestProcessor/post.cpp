#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

#include "Connection.hpp"
#include "Logger.hpp"
#include "Response.hpp"
#include "file_manager.hpp"

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
            if (std::rename(_request.body_path().c_str(), resource_path.c_str()) < 0) {
                Logger(LOG_ERROR) << "[process_post_request] - rename: " << strerror(errno);
                _response = error_response(500, false);
                return;
            }
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
                close(fd);
                return;
            }
            close(fd);
        }
        _response.set_code(200);  // TODO return 30x See Other response
        _response.set_response_string("OK");
        _response.set_header("Content-Length", "0");
    }
}
