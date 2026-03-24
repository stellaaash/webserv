#ifndef CONFIG_LEXER_HPP
#define CONFIG_LEXER_HPP

#include <fstream>
#include <string>
#include <vector>

enum TokenType { WORD, OPENING_BRACE, CLOSING_BRACE, SEMICOLON };

struct Token {
    enum TokenType type;
    std::string    word;
};

typedef std::vector<Token>::const_iterator TokenIterator;

std::vector<Token> lex_config(std::ifstream& file_stream);

#endif
