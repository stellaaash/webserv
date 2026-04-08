#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <sstream>

enum LogLevel { DEBUG_LOG, GENERAL_LOG, ERROR_LOG };

/**
 * Usage: Logger(DEBUG_LOG) << "string " << num << " string"
 */
class Logger {
public:
    Logger(LogLevel level);
    ~Logger();

    template <typename T>
    Logger& operator<<(const T& value) {
        _oss << value;
        return *this;
    }

private:
    LogLevel           _level;
    std::ostringstream _oss;
};

#endif
