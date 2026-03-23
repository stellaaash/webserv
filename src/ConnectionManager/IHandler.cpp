#include "IHandler.hpp"

IHandler::~IHandler() {}

bool IHandler::is_timed_out() const {
    return false;
}
