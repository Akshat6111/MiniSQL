#include <iostream>

#include "lexer/lexer.h"
#include "lexer/token.h"

using minisql::Lexer;
using minisql::Token;
using minisql::TokenTypeToString;

void PrintTokens(const std::string& sql) {
    std::cout << "SQL: " << sql << "\n";
    Lexer lexer(sql);
    std::vector<Token> tokens = lexer.Tokenize();
    for (const Token& token : tokens) {
        std::cout << "  " << TokenTypeToString(token.type);
        if (!token.lexeme.empty()) {
            std::cout << " (\"" << token.lexeme << "\")";
        }
        std::cout << "  [line " << token.line << ", col " << token.column << "]\n";
    }
    std::cout << "\n";
}

int main() {
    PrintTokens("CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR);");
    PrintTokens("INSERT INTO users VALUES (1, 'Alice');");
    PrintTokens("SELECT * FROM users WHERE id = 1;");
    PrintTokens(
        "SELECT users.name, orders.total FROM users JOIN orders ON users.id = "
        "orders.user_id WHERE users.id >= 10;");
    return 0;
}
