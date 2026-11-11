#pragma once

#include "ast/ast.h"
#include "executor/row_context.h"
#include "storage/row.h"

namespace minisql {

class ExpressionEvaluator {
public:
    explicit ExpressionEvaluator(const RowContext& context);

    Value EvaluateValue(const Expression* expr) const;
    bool EvaluateBoolean(const Expression* expr) const;

private:
    const RowContext& context_;

    static int Compare(const Value& left, const Value& right);
    static bool EvaluateComparison(BinaryOperator op, const Value& left, const Value& right);
};

}  // namespace minisql
