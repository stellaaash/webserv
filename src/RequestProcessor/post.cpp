#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

#include "Connection.hpp"
#include "Logger.hpp"
#include "Response.hpp"
#include "cgi.hpp"
#include "file_manager.hpp"

void Connection::process_post_request(const FilePath& relative_path) {
    if (_request.config()->cgi.empty() == false) {
        FilePath cgi_path = resolve_path(relative_path, _request.config()->root);

        if (is_regular_file(cgi_path) == true) {
            Logger(LOG_DEBUG) << "[process_post_request] - CGI called : " << cgi_path;

            CgiProcess process = start_cgi(build_cgi_request(relative_path, _request));
            if (process.pid == -1) {
                _response = error_response(500, false);
                return;
            }

            _pending_cgi_process = process;
            _has_pending_cgi = true;
            return;
        }
    }

    if (_request.config()->upload_store.empty() == false) {
        FilePath upload_path = resolve_path(relative_path, _request.config()->upload_store);

        Logger(LOG_DEBUG) << "[process_post_request] - Upload called : " << upload_path;

        if (is_regular_file(upload_path) || is_directory(upload_path)) {
            // TODO Don't know if we'll do that one: 409 conflict is only returned after the client
            // already sent all of its file and the requesst was parsed, which is a waste of network
            // resources
            // In a perfect world, the parsing would say no to the request before allowing anything
            // from the body to arrive
            _response = error_response(409, false);
            return;
        }

        if (_request.is_body_spooled() == true) {
            if (std::rename(_request.body_path().c_str(), upload_path.c_str()) < 0) {
                Logger(LOG_ERROR) << "[process_post_request] - rename: " << strerror(errno);
                _response = error_response(500, false);
                return;
            }
        } else {
            int fd = create_file(upload_path);
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

        const std::string uploaded_resource = _request.config()->name + "/" + relative_path;
        _response.set_code(303);
        _response.set_response_string("See Other");
        _response.set_header("Content-Length", "0");
        _response.set_header("Location", uploaded_resource);
        return;
    }

    _response = error_response(404, true);
}
