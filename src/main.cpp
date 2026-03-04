#include <fstream>
#include <iostream>

#include "ConfigParser.hpp"
#include "config.hpp"

int main() {
    std::ifstream config_file("./config/1.conf");

    try {
        Config config = ConfigParser::parse_file(config_file);

    } catch (const ConfigParser::Parser::ParserError& e) {
        std::cerr << "[!] - Error occurred during parsing: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
