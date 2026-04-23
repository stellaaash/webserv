#include "Connection.hpp"
#include "Request.hpp"
#include "Response.hpp"

ResponseStatus Connection::process_request() {
    _request.set_status(REQ_PROCESSED);
    return _response.status();
}
