#include <cstdio>

#include "Connection.hpp"
#include "Response.hpp"
#include "file_manager.hpp"

void Connection::process_delete_request(const FilePath& resource_path) {
    // TODO Forbidden function?? Hello?? I'm so confused right now
    // This is the only way to remove a file, and guess what? It's not in the allowed functions list
    // This makes me rethink what the allowed functions list actually means, whether we can use
    // other syscalls like rename, getcwd etc
    if (remove(resource_path.c_str()) < 0) {
        // TODO check the exact errno values, and create a better error response
        _response = error_response(500, false);
        return;
    }
    _response.set_code(200);
    _response.set_response_string("OK");
    _response.set_header("Content-Length", "0");
    return;
}
