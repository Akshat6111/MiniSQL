#pragma once

#include <memory>
#include <vector>

#include "ast/ast.h"
#include "lexer/token.h"

namespace minisql {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    std::unique_ptr<Statement> ParseStatement();

private:
    std::vector<Token> tokens_;
    size_t position_;

    const Token& Peek() const;
    const Token& Previous() const;
    const Token& Advance();
    bool Check(TokenType type) const;
    bool Match(TokenType type);
    const Token& Expect(TokenType type, const std::string& errorMessage);
    bool IsAtEnd() const;
    [[noreturn]] void Error(const std::string& message) const;

    std::unique_ptr<Statement> ParseCreateStatement();
    std::unique_ptr<Statement> ParseInsertStatement();
    std::unique_ptr<Statement> ParseSelectStatement();

    ColumnDefinition ParseColumnDefinition();
    AstColumnType ParseColumnType();

    std::vector<std::string> ParseSelectColumnList(bool& selectAll);
    std::string ParseQualifiedIdentifier();

    std::unique_ptr<JoinClause> ParseOptionalJoinClause();
    std::unique_ptr<Expression> ParseOptionalWhereClause();

    std::unique_ptr<Expression> ParseOrExpression();
    std::unique_ptr<Expression> ParseAndExpression();
    std::unique_ptr<Expression> ParseComparisonExpression();
    std::unique_ptr<Expression> ParsePrimaryExpression();
    std::unique_ptr<Expression> ParseLiteralExpression();

    static bool IsComparisonOperator(TokenType type);
    static BinaryOperator TokenToBinaryOperator(TokenType type);
};

}  // namespace minisql
