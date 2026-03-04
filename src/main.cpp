#include <fstream>
#include <iostream>

#include "ConfigParser.hpp"
#include "config.hpp"

int main(int argc, char** argv) {
    if (argc != 2) return 1;

    std::ifstream config_file(argv[1]);

    try {
        Config config = ConfigParser::parse_file(config_file);

    } catch (const ConfigParser::Parser::ParserError& e) {
        std::cerr << "[!] - Error occurred during parsing: " << e.what() << std::endl;
        return 2;
    }
    return 0;
}
