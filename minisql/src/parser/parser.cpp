#include "parser/parser.h"

#include "common/exceptions.h"

namespace minisql {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)), position_(0) {}

const Token& Parser::Peek() const {
    return tokens_[position_];
}

const Token& Parser::Previous() const {
    return tokens_[position_ - 1];
}

const Token& Parser::Advance() {
    if (!IsAtEnd()) {
        position_++;
    }
    return Previous();
}

bool Parser::Check(TokenType type) const {
    if (IsAtEnd()) {
        return type == TokenType::END_OF_FILE;
    }
    return Peek().type == type;
}

bool Parser::Match(TokenType type) {
    if (Check(type)) {
        Advance();
        return true;
    }
    return false;
}

const Token& Parser::Expect(TokenType type, const std::string& errorMessage) {
    if (Check(type)) {
        return Advance();
    }
    Error(errorMessage);
}

bool Parser::IsAtEnd() const {
    return Peek().type == TokenType::END_OF_FILE;
}

void Parser::Error(const std::string& message) const {
    const Token& token = Peek();
    throw MiniSqlException("Parse error at line " + std::to_string(token.line) + ", column " +
                            std::to_string(token.column) + ": " + message + " (got '" + token.lexeme +
                            "')");
}

std::unique_ptr<Statement> Parser::ParseStatement() {
    std::unique_ptr<Statement> statement;

    if (Check(TokenType::CREATE)) {
        statement = ParseCreateStatement();
    } else if (Check(TokenType::INSERT)) {
        statement = ParseInsertStatement();
    } else if (Check(TokenType::SELECT)) {
        statement = ParseSelectStatement();
    } else {
        Error("expected CREATE, INSERT, or SELECT");
    }

    Match(TokenType::SEMICOLON);
    Expect(TokenType::END_OF_FILE, "unexpected tokens after statement");
    return statement;
}

std::unique_ptr<Statement> Parser::ParseCreateStatement() {
    Expect(TokenType::CREATE, "expected CREATE");
    Expect(TokenType::TABLE, "expected TABLE after CREATE");
    Token nameToken = Expect(TokenType::IDENTIFIER, "expected table name");
    Expect(TokenType::LEFT_PAREN, "expected '(' after table name");

    std::vector<ColumnDefinition> columns;
    columns.push_back(ParseColumnDefinition());
    while (Match(TokenType::COMMA)) {
        columns.push_back(ParseColumnDefinition());
    }

    Expect(TokenType::RIGHT_PAREN, "expected ')' after column list");
    return std::make_unique<CreateStatement>(nameToken.lexeme, std::move(columns));
}

ColumnDefinition Parser::ParseColumnDefinition() {
    Token nameToken = Expect(TokenType::IDENTIFIER, "expected column name");
    AstColumnType type = ParseColumnType();

    bool isPrimaryKey = false;
    if (Match(TokenType::PRIMARY)) {
        Expect(TokenType::KEY, "expected KEY after PRIMARY");
        isPrimaryKey = true;
    }

    return ColumnDefinition{nameToken.lexeme, type, isPrimaryKey};
}

AstColumnType Parser::ParseColumnType() {
    if (Match(TokenType::INT)) {
        return AstColumnType::INT;
    }
    if (Match(TokenType::VARCHAR)) {
        return AstColumnType::VARCHAR;
    }
    Error("expected column type INT or VARCHAR");
}

std::unique_ptr<Statement> Parser::ParseInsertStatement() {
    Expect(TokenType::INSERT, "expected INSERT");
    Expect(TokenType::INTO, "expected INTO after INSERT");
    Token nameToken = Expect(TokenType::IDENTIFIER, "expected table name");
    Expect(TokenType::VALUES, "expected VALUES after table name");
    Expect(TokenType::LEFT_PAREN, "expected '(' before values");

    std::vector<std::unique_ptr<Expression>> values;
    values.push_back(ParseLiteralExpression());
    while (Match(TokenType::COMMA)) {
        values.push_back(ParseLiteralExpression());
    }

    Expect(TokenType::RIGHT_PAREN, "expected ')' after values");
    return std::make_unique<InsertStatement>(nameToken.lexeme, std::move(values));
}

std::unique_ptr<Statement> Parser::ParseSelectStatement() {
    Expect(TokenType::SELECT, "expected SELECT");

    bool selectAll = false;
    std::vector<std::string> columns = ParseSelectColumnList(selectAll);

    Expect(TokenType::FROM, "expected FROM after column list");
    Token tableToken = Expect(TokenType::IDENTIFIER, "expected table name after FROM");

    auto statement = std::make_unique<SelectStatement>(std::move(columns), selectAll, tableToken.lexeme);
    statement->join = ParseOptionalJoinClause();
    statement->whereClause = ParseOptionalWhereClause();
    return statement;
}

std::vector<std::string> Parser::ParseSelectColumnList(bool& selectAll) {
    std::vector<std::string> columns;

    if (Match(TokenType::STAR)) {
        selectAll = true;
        return columns;
    }

    columns.push_back(ParseQualifiedIdentifier());
    while (Match(TokenType::COMMA)) {
        columns.push_back(ParseQualifiedIdentifier());
    }
    return columns;
}

std::string Parser::ParseQualifiedIdentifier() {
    Token first = Expect(TokenType::IDENTIFIER, "expected identifier");
    if (Match(TokenType::DOT)) {
        Token second = Expect(TokenType::IDENTIFIER, "expected identifier after '.'");
        return first.lexeme + "." + second.lexeme;
    }
    return first.lexeme;
}

std::unique_ptr<JoinClause> Parser::ParseOptionalJoinClause() {
    if (!Match(TokenType::JOIN)) {
        return nullptr;
    }

    auto join = std::make_unique<JoinClause>();
    join->tableName = Expect(TokenType::IDENTIFIER, "expected table name after JOIN").lexeme;
    Expect(TokenType::ON, "expected ON after JOIN table name");

    join->leftTable = Expect(TokenType::IDENTIFIER, "expected table name in ON clause").lexeme;
    Expect(TokenType::DOT, "expected '.' after table name in ON clause");
    join->leftColumn = Expect(TokenType::IDENTIFIER, "expected column name in ON clause").lexeme;

    Expect(TokenType::EQUALS, "expected '=' in ON clause");

    join->rightTable = Expect(TokenType::IDENTIFIER, "expected table name in ON clause").lexeme;
    Expect(TokenType::DOT, "expected '.' after table name in ON clause");
    join->rightColumn = Expect(TokenType::IDENTIFIER, "expected column name in ON clause").lexeme;

    return join;
}

std::unique_ptr<Expression> Parser::ParseOptionalWhereClause() {
    if (!Match(TokenType::WHERE)) {
        return nullptr;
    }
    return ParseOrExpression();
}

std::unique_ptr<Expression> Parser::ParseOrExpression() {
    std::unique_ptr<Expression> left = ParseAndExpression();
    while (Match(TokenType::OR)) {
        std::unique_ptr<Expression> right = ParseAndExpression();
        left = std::make_unique<BinaryExpression>(std::move(left), BinaryOperator::OR, std::move(right));
    }
    return left;
}

std::unique_ptr<Expression> Parser::ParseAndExpression() {
    std::unique_ptr<Expression> left = ParseComparisonExpression();
    while (Match(TokenType::AND)) {
        std::unique_ptr<Expression> right = ParseComparisonExpression();
        left = std::make_unique<BinaryExpression>(std::move(left), BinaryOperator::AND, std::move(right));
    }
    return left;
}

bool Parser::IsComparisonOperator(TokenType type) {
    switch (type) {
        case TokenType::EQUALS:
        case TokenType::NOT_EQUALS:
        case TokenType::LESS_THAN:
        case TokenType::GREATER_THAN:
        case TokenType::LESS_EQUAL:
        case TokenType::GREATER_EQUAL:
            return true;
        default:
            return false;
    }
}

BinaryOperator Parser::TokenToBinaryOperator(TokenType type) {
    switch (type) {
        case TokenType::EQUALS: return BinaryOperator::EQUALS;
        case TokenType::NOT_EQUALS: return BinaryOperator::NOT_EQUALS;
        case TokenType::LESS_THAN: return BinaryOperator::LESS_THAN;
        case TokenType::GREATER_THAN: return BinaryOperator::GREATER_THAN;
        case TokenType::LESS_EQUAL: return BinaryOperator::LESS_EQUAL;
        case TokenType::GREATER_EQUAL: return BinaryOperator::GREATER_EQUAL;
        default:
            throw MiniSqlException("Internal error: token is not a comparison operator");
    }
}

std::unique_ptr<Expression> Parser::ParseComparisonExpression() {
    std::unique_ptr<Expression> left = ParsePrimaryExpression();

    if (IsComparisonOperator(Peek().type)) {
        BinaryOperator op = TokenToBinaryOperator(Advance().type);
        std::unique_ptr<Expression> right = ParsePrimaryExpression();
        return std::make_unique<BinaryExpression>(std::move(left), op, std::move(right));
    }

    return left;
}

std::unique_ptr<Expression> Parser::ParsePrimaryExpression() {
    if (Check(TokenType::NUMBER) || Check(TokenType::STRING)) {
        return ParseLiteralExpression();
    }

    if (Check(TokenType::IDENTIFIER)) {
        Token first = Advance();
        std::string tableName;
        std::string columnName = first.lexeme;
        if (Match(TokenType::DOT)) {
            tableName = first.lexeme;
            columnName = Expect(TokenType::IDENTIFIER, "expected column name after '.'").lexeme;
        }
        return std::make_unique<ColumnReferenceExpression>(tableName, columnName);
    }

    Error("expected a column reference or literal value");
}

std::unique_ptr<Expression> Parser::ParseLiteralExpression() {
    if (Check(TokenType::NUMBER)) {
        Token token = Advance();
        return std::make_unique<LiteralExpression>(LiteralType::INTEGER, token.lexeme);
    }
    if (Check(TokenType::STRING)) {
        Token token = Advance();
        return std::make_unique<LiteralExpression>(LiteralType::STRING, token.lexeme);
    }
    Error("expected a number or string literal");
}

}  // namespace minisql
