#include "ast/ast.h"

#include <utility>

namespace minisql {

ColumnReferenceExpression::ColumnReferenceExpression(std::string tableName, std::string columnName)
    : tableName(std::move(tableName)), columnName(std::move(columnName)) {}

ExpressionType ColumnReferenceExpression::GetType() const {
    return ExpressionType::COLUMN_REFERENCE;
}

LiteralExpression::LiteralExpression(LiteralType literalType, std::string value)
    : literalType(literalType), value(std::move(value)) {}

ExpressionType LiteralExpression::GetType() const {
    return ExpressionType::LITERAL;
}

BinaryExpression::BinaryExpression(std::unique_ptr<Expression> left, BinaryOperator op,
                                    std::unique_ptr<Expression> right)
    : left(std::move(left)), op(op), right(std::move(right)) {}

ExpressionType BinaryExpression::GetType() const {
    return ExpressionType::BINARY;
}

CreateStatement::CreateStatement(std::string tableName, std::vector<ColumnDefinition> columns)
    : tableName(std::move(tableName)), columns(std::move(columns)) {}

StatementType CreateStatement::GetType() const {
    return StatementType::CREATE_TABLE;
}

InsertStatement::InsertStatement(std::string tableName, std::vector<std::unique_ptr<Expression>> values)
    : tableName(std::move(tableName)), values(std::move(values)) {}

StatementType InsertStatement::GetType() const {
    return StatementType::INSERT;
}

SelectStatement::SelectStatement(std::vector<std::string> columns, bool selectAll, std::string tableName)
    : columns(std::move(columns)), selectAll(selectAll), tableName(std::move(tableName)) {}

StatementType SelectStatement::GetType() const {
    return StatementType::SELECT;
}

}  // namespace minisql
