#pragma once

#include <memory>
#include <string>
#include <vector>

namespace minisql {

enum class AstColumnType { INT, VARCHAR };

struct ColumnDefinition {
    std::string name;
    AstColumnType type;
    bool isPrimaryKey;
};

enum class ExpressionType { COLUMN_REFERENCE, LITERAL, BINARY };

class Expression {
public:
    virtual ~Expression() = default;
    virtual ExpressionType GetType() const = 0;
};

class ColumnReferenceExpression : public Expression {
public:
    ColumnReferenceExpression(std::string tableName, std::string columnName);
    ExpressionType GetType() const override;

    std::string tableName;
    std::string columnName;
};

enum class LiteralType { INTEGER, STRING };

class LiteralExpression : public Expression {
public:
    LiteralExpression(LiteralType literalType, std::string value);
    ExpressionType GetType() const override;

    LiteralType literalType;
    std::string value;
};

enum class BinaryOperator {
    EQUALS,
    NOT_EQUALS,
    LESS_THAN,
    GREATER_THAN,
    LESS_EQUAL,
    GREATER_EQUAL,
    AND,
    OR
};

class BinaryExpression : public Expression {
public:
    BinaryExpression(std::unique_ptr<Expression> left, BinaryOperator op, std::unique_ptr<Expression> right);
    ExpressionType GetType() const override;

    std::unique_ptr<Expression> left;
    BinaryOperator op;
    std::unique_ptr<Expression> right;
};

enum class StatementType { CREATE_TABLE, INSERT, SELECT };

class Statement {
public:
    virtual ~Statement() = default;
    virtual StatementType GetType() const = 0;
};

class CreateStatement : public Statement {
public:
    CreateStatement(std::string tableName, std::vector<ColumnDefinition> columns);
    StatementType GetType() const override;

    std::string tableName;
    std::vector<ColumnDefinition> columns;
};

class InsertStatement : public Statement {
public:
    InsertStatement(std::string tableName, std::vector<std::unique_ptr<Expression>> values);
    StatementType GetType() const override;

    std::string tableName;
    std::vector<std::unique_ptr<Expression>> values;
};

struct JoinClause {
    std::string tableName;
    std::string leftTable;
    std::string leftColumn;
    std::string rightTable;
    std::string rightColumn;
};

class SelectStatement : public Statement {
public:
    SelectStatement(std::vector<std::string> columns, bool selectAll, std::string tableName);
    StatementType GetType() const override;

    std::vector<std::string> columns;
    bool selectAll;
    std::string tableName;
    std::unique_ptr<JoinClause> join;
    std::unique_ptr<Expression> whereClause;
};

}  // namespace minisql
