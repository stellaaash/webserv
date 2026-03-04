#ifndef CONFIGLEXER_HPP
#define CONFIGLEXER_HPP

#include <fstream>
#include <string>
#include <vector>
namespace ConfigParser {
namespace Lexer {
enum TokenType { WORD, OPENING_BRACE, CLOSING_BRACE, SEMICOLON };

struct Token {
    enum TokenType type;
    std::string    word;
};

typedef std::vector<Token>::const_iterator token_iterator;

std::vector<Token> lex_config(std::ifstream& file_stream);
}  // namespace Lexer
}  // namespace ConfigParser

#endif
