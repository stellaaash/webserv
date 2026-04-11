#include "Logger.hpp"

#include <ctime>
#include <fstream>
#include <string>

static std::string& get_log_directory() {
    static std::string log_dir;
    return log_dir;
}

/**
 * @brief The functions Open and return the log files in "append" mode.
 */
static std::ofstream& get_debug_file() {
    static std::ofstream ofs((get_log_directory() + "/log_debug.log").c_str(), std::ios::app);
    return ofs;
}

static std::ofstream& get_general_file() {
    static std::ofstream ofs((get_log_directory() + "/log_general.log").c_str(), std::ios::app);
    return ofs;
}

static std::ofstream& get_error_file() {
    static std::ofstream ofs((get_log_directory() + "/log_error.log").c_str(), std::ios::app);
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
    if (level >= LOG_DEBUG) write_log(get_debug_file(), msg);
    if (level >= LOG_GENERAL) write_log(get_general_file(), msg);
    if (level >= LOG_ERROR) write_log(get_error_file(), msg);
}

void logger_init(const std::string& log_dir) {
    if (log_dir.empty())
        get_log_directory() = ".";
    else
        get_log_directory() = log_dir;
    std::ofstream((get_log_directory() + "/log_debug.log").c_str(), std::ios::trunc).close();
    std::ofstream((get_log_directory() + "/log_general.log").c_str(), std::ios::trunc).close();
    std::ofstream((get_log_directory() + "/log_error.log").c_str(), std::ios::trunc).close();
}

Logger::Logger(LogLevel level) : _level(level) {}

Logger::~Logger() {
    dispatch_log(_level, _oss.str());
}
