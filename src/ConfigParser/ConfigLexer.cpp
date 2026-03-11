#include <ConfigLexer.hpp>
#include <cctype>
#include <fstream>
#include <ios>
#include <iostream>
#include <limits>
#include <vector>

/**
 * @brief Determines whether a character has a special meaning.
 * Characters with special meaning are: ;{}#.
 */
bool is_special(char c) {
    return (c == ';' || c == '{' || c == '}' || c == '#');
}

/**
 * @brief Determines wether a character is considered "inside a lexer word".
 * This means that the character should be printable, and not one of the so-called
 * "special" characters.
 */
bool is_word(char c) {
    return (std::isprint(c) && c != ' ' && !is_special(c));
}

/**
 * @brief Processes a file and transforms it in a series of tokens, which are either
 * words or special characters (braces and semicolons, comments are ignored).
 */
std::vector<Token> lex_config(std::ifstream& file_stream) {
    std::vector<Token> tokens;
    Token              token;

    while (!file_stream.eof()) {
        char c = static_cast<char>(file_stream.get());

        // Add all word characters to the current token
        if (is_word(c)) {
            token.word.push_back(c);
        } else if (!token.word.empty()) {
            // If the character isn't part of a word, then we've reached the end of the last one
            token.type = WORD;
            tokens.push_back(token);
            std::clog << "[!] - Extracted token " << token.word << " of type " << token.type
                      << std::endl;
            token.word.erase();
        }

        if (is_special(c)) {
            if (c == '#') {  // Ignore comments
                file_stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }
            token.word = c;
            if (c == ';') {
                token.type = SEMICOLON;
            } else if (c == '{') {
                token.type = OPENING_BRACE;
            } else if (c == '}') {
                token.type = CLOSING_BRACE;
            }
            tokens.push_back(token);
            std::clog << "[!] - Extracted token " << token.word << " of type " << token.type
                      << std::endl;
            token.word.erase();
        }
    }

    return tokens;
}
