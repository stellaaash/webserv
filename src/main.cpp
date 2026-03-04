#include <fstream>
#include <vector>

#include "ConfigLexer.hpp"
#include "config.hpp"

int main() {
    Config config = mock_config();

    std::ifstream config_file("./config/1.conf");

    std::vector<ConfigParser::Lexer::Token> tokens = ConfigParser::Lexer::lex_config(config_file);
    return 0;
}
