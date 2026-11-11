#pragma once

#include <string>
#include <vector>

#include "lexer/token.h"

namespace minisql {

class Lexer {
public:
    explicit Lexer(std::string source);

    std::vector<Token> Tokenize();

private:
    std::string source_;
    size_t position_;
    int line_;
    int column_;

    bool IsAtEnd() const;
    char Peek() const;
    char PeekNext() const;
    char Advance();
    bool Match(char expected);

    void SkipWhitespaceAndComments();

    Token NextToken();
    Token MakeToken(TokenType type, const std::string& lexeme, int startColumn) const;

    Token ReadIdentifierOrKeyword();
    Token ReadNumber();
    Token ReadString();

    static TokenType LookupKeyword(const std::string& text);
};

}  // namespace minisql
