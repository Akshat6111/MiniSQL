#pragma once

#include <string>

namespace minisql {

enum class TokenType {
    // Keywords
    SELECT,
    INSERT,
    INTO,
    VALUES,
    CREATE,
    TABLE,
    WHERE,
    JOIN,
    ON,
    FROM,
    AND,
    OR,
    PRIMARY,
    KEY,
    INT,
    VARCHAR,

    // Literals
    IDENTIFIER,
    NUMBER,
    STRING,

    // Operators
    EQUALS,
    NOT_EQUALS,
    LESS_THAN,
    GREATER_THAN,
    LESS_EQUAL,
    GREATER_EQUAL,

    // Punctuation
    LEFT_PAREN,
    RIGHT_PAREN,
    COMMA,
    SEMICOLON,
    STAR,
    DOT,

    // Special
    END_OF_FILE,
    INVALID
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;

    Token(TokenType type, std::string lexeme, int line, int column)
        : type(type), lexeme(std::move(lexeme)), line(line), column(column) {}
};

std::string TokenTypeToString(TokenType type);

}  // namespace minisql
