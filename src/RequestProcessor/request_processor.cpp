#include <cassert>
#include <cerrno>
#include <cstdio>

#include "Connection.hpp"
#include "Logger.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "file_manager.hpp"

// TODO Note to self: if this architecture works out (request processor as part of Connection),
// standardize it with the Request Parser, too
// IT'S NOT WORKING OUT THIS IS SO UGLY AND INCOMPREHENSIBLE I HATE IT

void Connection::process_get_request(const FilePath& resource_path) {
    // Fetch the resource or generate content
    if (is_directory(resource_path) == true) {
        if (_request.config()->autoindex == true) {
            _response.append_body(create_listing(_request.config()->root));
            _response.set_code(200);
            _response.set_response_string("OK");
        } else {
            _response.set_code(404);
            _response.set_status(RS_ERROR);
        }
        return;
    } else if (!is_regular_file(resource_path)) {
        _response.set_code(404);
        _response.set_status(RS_ERROR);
        return;
    }

    int fd = fetch_file(resource_path);
    if (fd < 0) {
        perror("[process_request] - fetch_file");
        if (errno == EACCES)
            _response.set_code(403);
        else if (errno == ENOENT)
            _response.set_code(404);
        else if (errno == EINVAL || errno == ENAMETOOLONG) {
            Logger(LOG_DEBUG) << "Bad request - invalid resource path";
            _response.set_code(400);
        } else
            _response.set_code(500);
        _response.set_status(RS_ERROR);
        return;
    }
    // TODO Throw or return error page (we could technically throw a Response)

    _response.set_fd(fd);
    _response.set_code(200);
    _response.set_response_string("OK");
    // TODO Set header, MIME types??
}

void Connection::process_post_request() {
    // TODO Add file or launch CGI
    _response.set_code(501);
    _response.set_status(RS_ERROR);
}

void Connection::process_delete_request() {
    // TODO Remove file
    _response.set_code(501);
    _response.set_status(RS_ERROR);
}

/**
 * @brief Processes a Request, returning an appropriate Response object.
 *
 * @description The Response object will either contain a valid file descriptor to read from, or
 * a filled body string containing the generated content.
 */
void Connection::process_request() {
    const ConfigLocation* const config = _request.config();
    FilePath                    resource_path;
    FilePath                    relative_path;

    assert(config && "Config pointer valid");

    _response.set_version(1, 1);

    // The relative path is the path from the root of the location to the resource
    relative_path = _request.target().substr(config->name.length(), _request.target().npos);
    Logger(LOG_DEBUG) << "[!] - Relative path: " << relative_path;

    // Isolate the resource needed
    if (relative_path.empty() && !config->index.empty()) {
        resource_path = config->index;
    } else {
        resource_path = config->root + "/" + relative_path;
    }

    if (_request.config()->allowed_methods.find(_request.method()) ==
        _request.config()->allowed_methods.end()) {
        _response.set_code(405);
        _response.set_status(RS_ERROR);
        return;
    }

    Logger(LOG_DEBUG) << "[REPRO] - Fetching resource: " << resource_path;

    if (_request.method() == GET) {
        process_get_request(resource_path);
    } else if (_request.method() == POST) {
        process_post_request();
    } else if (_request.method() == DELETE) {
        process_delete_request();
    } else {
        _response.set_code(501);
        _response.set_status(RS_ERROR);
    }
    if (_response.status() == RS_ERROR) return;

    std::stringstream length;
    if (_response.body().empty() == false)
        length << _response.body().length();
    else
        length << file_length(resource_path);
    _response.set_header("Content-Length", length.str());

    // TODO Fetch the error page if needed and configured
}
