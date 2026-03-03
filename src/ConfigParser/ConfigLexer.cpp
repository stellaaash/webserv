#include <ConfigLexer.hpp>
#include <fstream>
#include <ios>
#include <iostream>
#include <limits>
#include <vector>

namespace ConfigParser {
namespace Lexer {
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
