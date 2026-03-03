#include <ConfigLexer.hpp>
#include <fstream>
#include <ios>
#include <iostream>
#include <limits>
#include <vector>

namespace ConfigParser {
namespace Lexer {
/**
 * @brief Determines whether a character has a special meaning.
 * Characters with special meaning are: ;{}#.
 */
bool is_special(char c) {
    return (c == ';' || c == '{' || c == '}' || c == '#');
}

std::vector<Token> lexConfig(std::ifstream& file_stream) {
    std::string        word;
    std::vector<Token> tokens;

    while (!file_stream.eof()) {
        file_stream >> word;
        if (word.find("#") != word.npos) {
            file_stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        std::cout << "[!] - Extracted word " << word << std::endl;
    }

    return tokens;
}
}  // namespace Lexer
}  // namespace ConfigParser
