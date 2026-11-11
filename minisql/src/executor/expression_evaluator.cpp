#include "executor/expression_evaluator.h"

#include "common/exceptions.h"

namespace minisql {

ExpressionEvaluator::ExpressionEvaluator(const RowContext& context) : context_(context) {}

Value ExpressionEvaluator::EvaluateValue(const Expression* expr) const {
    switch (expr->GetType()) {
        case ExpressionType::LITERAL: {
            const auto* literal = static_cast<const LiteralExpression*>(expr);
            if (literal->literalType == LiteralType::INTEGER) {
                return Value(static_cast<int32_t>(std::stol(literal->value)));
            }
            return Value(literal->value);
        }
        case ExpressionType::COLUMN_REFERENCE: {
            const auto* ref = static_cast<const ColumnReferenceExpression*>(expr);
            return context_.Resolve(ref->tableName, ref->columnName);
        }
        case ExpressionType::BINARY:
            throw MiniSqlException("Expected a column or literal, got a boolean expression");
    }
    throw MiniSqlException("Unhandled expression type");
}

int ExpressionEvaluator::Compare(const Value& left, const Value& right) {
    if (std::holds_alternative<int32_t>(left) && std::holds_alternative<int32_t>(right)) {
        int32_t a = std::get<int32_t>(left);
        int32_t b = std::get<int32_t>(right);
        return (a < b) ? -1 : (a > b ? 1 : 0);
    }
    if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right)) {
        return std::get<std::string>(left).compare(std::get<std::string>(right));
    }
    throw MiniSqlException("Cannot compare values of different types (INT vs VARCHAR)");
}

bool ExpressionEvaluator::EvaluateComparison(BinaryOperator op, const Value& left, const Value& right) {
    int cmp = Compare(left, right);
    switch (op) {
        case BinaryOperator::EQUALS: return cmp == 0;
        case BinaryOperator::NOT_EQUALS: return cmp != 0;
        case BinaryOperator::LESS_THAN: return cmp < 0;
        case BinaryOperator::GREATER_THAN: return cmp > 0;
        case BinaryOperator::LESS_EQUAL: return cmp <= 0;
        case BinaryOperator::GREATER_EQUAL: return cmp >= 0;
        default:
            throw MiniSqlException("Internal error: not a comparison operator");
    }
}

bool ExpressionEvaluator::EvaluateBoolean(const Expression* expr) const {
    if (expr->GetType() != ExpressionType::BINARY) {
        throw MiniSqlException("WHERE clause must evaluate to a boolean expression");
    }

    const auto* binary = static_cast<const BinaryExpression*>(expr);
    if (binary->op == BinaryOperator::AND) {
        return EvaluateBoolean(binary->left.get()) && EvaluateBoolean(binary->right.get());
    }
    if (binary->op == BinaryOperator::OR) {
        return EvaluateBoolean(binary->left.get()) || EvaluateBoolean(binary->right.get());
    }

    Value left = EvaluateValue(binary->left.get());
    Value right = EvaluateValue(binary->right.get());
    return EvaluateComparison(binary->op, left, right);
}

}  // namespace minisql
