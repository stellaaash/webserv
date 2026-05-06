#include "config.hpp"

// If nothing is provided in the location (except for the root), no default values are present.
// This would mean that you can't technically interact with the location, since no allowed
// methods have been defined. We won't set GET as default, since maybe that's not what the user
// wants, who knows.
ConfigLocation::ConfigLocation()
    : allowed_methods(), autoindex(false), cgi(), index(), redirect(), root(), upload_store() {}

ConfigServer::ConfigServer()
    : client_max_body_size(0), error_page(), listen(), location(), timeout(10) {}

Config::Config() : error_log(), server() {}
