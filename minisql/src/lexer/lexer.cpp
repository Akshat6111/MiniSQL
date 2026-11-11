#include "lexer/lexer.h"

#include <cctype>
#include <unordered_map>

#include "common/exceptions.h"

namespace minisql {

namespace {

const std::unordered_map<std::string, TokenType>& KeywordTable() {
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"SELECT", TokenType::SELECT},   {"INSERT", TokenType::INSERT},
        {"INTO", TokenType::INTO},       {"VALUES", TokenType::VALUES},
        {"CREATE", TokenType::CREATE},   {"TABLE", TokenType::TABLE},
        {"WHERE", TokenType::WHERE},     {"JOIN", TokenType::JOIN},
        {"ON", TokenType::ON},           {"FROM", TokenType::FROM},
        {"AND", TokenType::AND},         {"OR", TokenType::OR},
        {"PRIMARY", TokenType::PRIMARY}, {"KEY", TokenType::KEY},
        {"INT", TokenType::INT},         {"VARCHAR", TokenType::VARCHAR},
    };
    return keywords;
}

std::string ToUpper(const std::string& text) {
    std::string result = text;
    for (char& c : result) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return result;
}

}  // namespace

Lexer::Lexer(std::string source) : source_(std::move(source)), position_(0), line_(1), column_(1) {}

bool Lexer::IsAtEnd() const {
    return position_ >= source_.size();
}

char Lexer::Peek() const {
    if (IsAtEnd()) {
        return '\0';
    }
    return source_[position_];
}

char Lexer::PeekNext() const {
    if (position_ + 1 >= source_.size()) {
        return '\0';
    }
    return source_[position_ + 1];
}

char Lexer::Advance() {
    char current = source_[position_];
    position_++;
    if (current == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    return current;
}

bool Lexer::Match(char expected) {
    if (IsAtEnd() || source_[position_] != expected) {
        return false;
    }
    Advance();
    return true;
}

void Lexer::SkipWhitespaceAndComments() {
    while (!IsAtEnd()) {
        char c = Peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            Advance();
        } else if (c == '-' && PeekNext() == '-') {
            while (!IsAtEnd() && Peek() != '\n') {
                Advance();
            }
        } else {
            break;
        }
    }
}

Token Lexer::MakeToken(TokenType type, const std::string& lexeme, int startColumn) const {
    return Token(type, lexeme, line_, startColumn);
}

Token Lexer::ReadIdentifierOrKeyword() {
    int startColumn = column_;
    size_t start = position_;
    while (!IsAtEnd() && (std::isalnum(static_cast<unsigned char>(Peek())) || Peek() == '_')) {
        Advance();
    }
    std::string text = source_.substr(start, position_ - start);
    TokenType type = LookupKeyword(text);
    return MakeToken(type, text, startColumn);
}

Token Lexer::ReadNumber() {
    int startColumn = column_;
    size_t start = position_;
    while (!IsAtEnd() && std::isdigit(static_cast<unsigned char>(Peek()))) {
        Advance();
    }
    if (!IsAtEnd() && Peek() == '.' && std::isdigit(static_cast<unsigned char>(PeekNext()))) {
        Advance();
        while (!IsAtEnd() && std::isdigit(static_cast<unsigned char>(Peek()))) {
            Advance();
        }
    }
    std::string text = source_.substr(start, position_ - start);
    return MakeToken(TokenType::NUMBER, text, startColumn);
}

Token Lexer::ReadString() {
    int startColumn = column_;
    Advance();  // consume opening quote
    size_t start = position_;
    while (!IsAtEnd() && Peek() != '\'') {
        Advance();
    }
    if (IsAtEnd()) {
        throw MiniSqlException("Unterminated string literal starting at line " + std::to_string(line_));
    }
    std::string text = source_.substr(start, position_ - start);
    Advance();  // consume closing quote
    return MakeToken(TokenType::STRING, text, startColumn);
}

TokenType Lexer::LookupKeyword(const std::string& text) {
    const auto& keywords = KeywordTable();
    auto it = keywords.find(ToUpper(text));
    if (it != keywords.end()) {
        return it->second;
    }
    return TokenType::IDENTIFIER;
}

Token Lexer::NextToken() {
    SkipWhitespaceAndComments();

    if (IsAtEnd()) {
        return MakeToken(TokenType::END_OF_FILE, "", column_);
    }

    int startColumn = column_;
    char c = Peek();

    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return ReadIdentifierOrKeyword();
    }
    if (std::isdigit(static_cast<unsigned char>(c))) {
        return ReadNumber();
    }
    if (c == '\'') {
        return ReadString();
    }

    Advance();
    switch (c) {
        case '(': return MakeToken(TokenType::LEFT_PAREN, "(", startColumn);
        case ')': return MakeToken(TokenType::RIGHT_PAREN, ")", startColumn);
        case ',': return MakeToken(TokenType::COMMA, ",", startColumn);
        case ';': return MakeToken(TokenType::SEMICOLON, ";", startColumn);
        case '*': return MakeToken(TokenType::STAR, "*", startColumn);
        case '.': return MakeToken(TokenType::DOT, ".", startColumn);
        case '=': return MakeToken(TokenType::EQUALS, "=", startColumn);
        case '<':
            if (Match('=')) {
                return MakeToken(TokenType::LESS_EQUAL, "<=", startColumn);
            }
            if (Match('>')) {
                return MakeToken(TokenType::NOT_EQUALS, "<>", startColumn);
            }
            return MakeToken(TokenType::LESS_THAN, "<", startColumn);
        case '>':
            if (Match('=')) {
                return MakeToken(TokenType::GREATER_EQUAL, ">=", startColumn);
            }
            return MakeToken(TokenType::GREATER_THAN, ">", startColumn);
        case '!':
            if (Match('=')) {
                return MakeToken(TokenType::NOT_EQUALS, "!=", startColumn);
            }
            break;
        default:
            break;
    }

    throw MiniSqlException("Unexpected character '" + std::string(1, c) + "' at line " +
                            std::to_string(line_) + ", column " + std::to_string(startColumn));
}

std::vector<Token> Lexer::Tokenize() {
    std::vector<Token> tokens;
    while (true) {
        Token token = NextToken();
        bool isEnd = token.type == TokenType::END_OF_FILE;
        tokens.push_back(std::move(token));
        if (isEnd) {
            break;
        }
    }
    return tokens;
}

}  // namespace minisql
