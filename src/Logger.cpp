#include "Logger.hpp"

#include <ctime>
#include <fstream>
#include <string>

/**
 * @brief The functions Open and return the log files in "append" mode.
 */
static std::ofstream& get_debug_file() {
    static std::ofstream ofs("log_debug.log", std::ios::app);
    return ofs;
}

static std::ofstream& get_general_file() {
    static std::ofstream ofs("log_general.log", std::ios::app);
    return ofs;
}

static std::ofstream& get_error_file() {
    static std::ofstream ofs("log_error.log", std::ios::app);
    return ofs;
}

static std::string get_timestamp() {
    std::time_t t = std::time(NULL);
    char        buf[64];

    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return std::string(buf);
}

static void write_log(std::ofstream& ofs, const std::string& msg) {
    ofs << "[" << get_timestamp() << "] " << msg << std::endl;
}

static void dispatch_log(LogLevel level, const std::string& msg) {
    if (level >= DEBUG_LOG) write_log(get_debug_file(), msg);
    if (level >= GENERAL_LOG) write_log(get_general_file(), msg);
    if (level >= ERROR_LOG) write_log(get_error_file(), msg);
}

Logger::Logger(LogLevel level) : _level(level) {}

Logger::~Logger() {
    dispatch_log(_level, _oss.str());
}
