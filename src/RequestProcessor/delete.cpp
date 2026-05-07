#include <cerrno>
#include <cstdio>

#include "Connection.hpp"
#include "Response.hpp"
#include "file_manager.hpp"

void Connection::process_delete_request(const FilePath& resource_path) {
    if (std::remove(resource_path.c_str()) < 0) {
        if (errno == ENOENT) {
            _response = error_response(404, false);
        } else if (errno == EACCES || errno == EPERM) {
            _response = error_response(403, false);
        } else if (errno == EINVAL) {
            _response = error_response(400, false);
        } else if (errno == EBUSY) {
            _response = error_response(503, false);
        } else {
            _response = error_response(500, false);
        }
        return;
    }
    _response.set_code(204);
    _response.set_response_string("No Content");
    return;
}
