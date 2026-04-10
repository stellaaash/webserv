#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <sstream>

enum LogLevel { LOG_DEBUG, LOG_GENERAL, LOG_ERROR };

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

void logger_init();

#endif
