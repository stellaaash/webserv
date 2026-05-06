#include <cstdio>

#include "Connection.hpp"
#include "Response.hpp"
#include "file_manager.hpp"

void Connection::process_delete_request(const FilePath& resource_path) {
    if (is_regular_file(resource_path) == false && is_directory(resource_path) == false) {
        _response = error_response(404, false);
        return;
    }
    if (std::remove(resource_path.c_str()) < 0) {
        // TODO check the exact errno values, and create a better error response
        _response = error_response(500, false);
        return;
    }
    _response.set_code(204);
    _response.set_response_string("No Content");
    return;
}
