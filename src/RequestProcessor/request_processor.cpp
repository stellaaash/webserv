#include "Connection.hpp"
#include "Response.hpp"

// TODO Move to dedicated request_processor.cpp
ResponseStatus Connection::process_request() {
    _response.set_status(RES_READY);
    return _response.status();
}
