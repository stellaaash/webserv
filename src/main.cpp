#include <fstream>

#include "ConfigParser.hpp"
#include "config.hpp"

int main() {
    std::ifstream config_file("./config/1.conf");

    Config config = ConfigParser::parse_file(config_file);

    return 0;
}
