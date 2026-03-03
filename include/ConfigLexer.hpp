#ifndef CONFIGLEXER_HPP
#define CONFIGLEXER_HPP

#include <string>
#include <vector>
namespace ConfigParser {
namespace Lexer {
enum TokenType { WORD, OPENING_BRACE, CLOSING_BRACE, SEMICOLON };

struct Token {
    enum TokenType type;
    std::string    word;
};

std::vector<Token> lexConfig(int file_descriptor);
}  // namespace Lexer
}  // namespace ConfigParser

#endif
