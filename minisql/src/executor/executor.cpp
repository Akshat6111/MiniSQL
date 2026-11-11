#include "executor/executor.h"

#include <algorithm>
#include <filesystem>
#include <limits>

#include "common/exceptions.h"
#include "executor/expression_evaluator.h"
#include "executor/row_context.h"

namespace minisql {

Executor::Executor(StorageEngine& storageEngine, std::string dataDirectory)
    : storageEngine_(storageEngine), dataDirectory_(std::move(dataDirectory)) {}

std::string Executor::IndexFilePath(const std::string& tableName, const std::string& columnName) const {
    return dataDirectory_ + "/" + tableName + "_" + columnName + ".idx";
}

ResultSet Executor::Execute(const Statement* statement) {
    switch (statement->GetType()) {
        case StatementType::CREATE_TABLE:
            return ExecuteCreate(static_cast<const CreateStatement*>(statement));
        case StatementType::INSERT:
            return ExecuteInsert(static_cast<const InsertStatement*>(statement));
        case StatementType::SELECT:
            return ExecuteSelect(static_cast<const SelectStatement*>(statement));
    }
    throw MiniSqlException("Unhandled statement type");
}

ResultSet Executor::ExecuteCreate(const CreateStatement* create) {
    std::vector<Column> columns;
    columns.reserve(create->columns.size());
    for (const ColumnDefinition& col : create->columns) {
        ColumnType storageType = (col.type == AstColumnType::INT) ? ColumnType::INT : ColumnType::VARCHAR;
        columns.push_back(Column{col.name, storageType, col.isPrimaryKey});
    }

    Schema schema(create->tableName, columns);
    storageEngine_.CreateTable(schema);

    bool indexed = false;
    for (const Column& column : columns) {
        if (column.isPrimaryKey && column.type == ColumnType::INT) {
            std::string indexPath = IndexFilePath(create->tableName, column.name);
            primaryKeyIndexes_[create->tableName] = BTree::Create(indexPath);
            primaryKeyColumnNames_[create->tableName] = column.name;
            indexed = true;
            break;
        }
    }
    if (!indexed) {
        primaryKeyIndexes_[create->tableName] = nullptr;
    }

    ResultSet result;
    result.columnNames = {"status"};
    result.rows = {{Value(std::string("table '" + create->tableName + "' created" +
                                       std::string(indexed ? " (indexed on primary key)" : "")))}};
    return result;
}

Value Executor::LiteralToValue(const LiteralExpression* literal, const Column& column) {
    if (column.type == ColumnType::INT) {
        if (literal->literalType != LiteralType::INTEGER) {
            throw MiniSqlException("Column '" + column.name + "' expects an INT value");
        }
        return Value(static_cast<int32_t>(std::stol(literal->value)));
    }
    if (literal->literalType != LiteralType::STRING) {
        throw MiniSqlException("Column '" + column.name + "' expects a VARCHAR value");
    }
    return Value(literal->value);
}

ResultSet Executor::ExecuteInsert(const InsertStatement* insert) {
    const Schema& schema = storageEngine_.GetSchema(insert->tableName);
    const std::vector<Column>& columns = schema.GetColumns();

    if (insert->values.size() != columns.size()) {
        throw MiniSqlException("INSERT has " + std::to_string(insert->values.size()) +
                                " values but table '" + insert->tableName + "' has " +
                                std::to_string(columns.size()) + " columns");
    }

    std::vector<Value> values;
    values.reserve(columns.size());
    for (size_t i = 0; i < columns.size(); i++) {
        const auto* literal = static_cast<const LiteralExpression*>(insert->values[i].get());
        values.push_back(LiteralToValue(literal, columns[i]));
    }

    Row row(values);
    RowLocation location = storageEngine_.InsertRow(insert->tableName, row);

    BTree* index = GetOrLoadIndex(insert->tableName);
    if (index != nullptr) {
        const std::string& pkColumn = primaryKeyColumnNames_[insert->tableName];
        int columnIndex = schema.FindColumnIndex(pkColumn);
        int32_t key = std::get<int32_t>(row.GetValue(static_cast<size_t>(columnIndex)));
        index->Insert(key, location);
    }

    ResultSet result;
    result.columnNames = {"status"};
    result.rows = {{Value(std::string("1 row inserted"))}};
    return result;
}

BTree* Executor::GetOrLoadIndex(const std::string& tableName) {
    auto it = primaryKeyIndexes_.find(tableName);
    if (it != primaryKeyIndexes_.end()) {
        return it->second.get();
    }

    const Schema& schema = storageEngine_.GetSchema(tableName);
    for (const Column& column : schema.GetColumns()) {
        if (column.isPrimaryKey && column.type == ColumnType::INT) {
            std::string indexPath = IndexFilePath(tableName, column.name);
            if (!std::filesystem::exists(indexPath)) {
                break;
            }
            std::unique_ptr<BTree> index = BTree::Open(indexPath);
            BTree* raw = index.get();
            primaryKeyIndexes_[tableName] = std::move(index);
            primaryKeyColumnNames_[tableName] = column.name;
            return raw;
        }
    }

    primaryKeyIndexes_[tableName] = nullptr;
    return nullptr;
}

std::optional<std::pair<int32_t, int32_t>> Executor::TryExtractIndexRange(
    const Expression* expr, const std::string& tableName, const std::string& pkColumn) const {
    if (expr->GetType() != ExpressionType::BINARY) {
        return std::nullopt;
    }
    const auto* binary = static_cast<const BinaryExpression*>(expr);

    if (binary->op == BinaryOperator::AND) {
        auto left = TryExtractIndexRange(binary->left.get(), tableName, pkColumn);
        auto right = TryExtractIndexRange(binary->right.get(), tableName, pkColumn);
        if (!left || !right) {
            return std::nullopt;
        }
        return std::make_pair(std::max(left->first, right->first), std::min(left->second, right->second));
    }

    if (binary->op == BinaryOperator::OR) {
        return std::nullopt;
    }

    const ColumnReferenceExpression* columnRef = nullptr;
    const LiteralExpression* literal = nullptr;
    bool columnOnLeft = true;

    if (binary->left->GetType() == ExpressionType::COLUMN_REFERENCE &&
        binary->right->GetType() == ExpressionType::LITERAL) {
        columnRef = static_cast<const ColumnReferenceExpression*>(binary->left.get());
        literal = static_cast<const LiteralExpression*>(binary->right.get());
        columnOnLeft = true;
    } else if (binary->left->GetType() == ExpressionType::LITERAL &&
               binary->right->GetType() == ExpressionType::COLUMN_REFERENCE) {
        literal = static_cast<const LiteralExpression*>(binary->left.get());
        columnRef = static_cast<const ColumnReferenceExpression*>(binary->right.get());
        columnOnLeft = false;
    } else {
        return std::nullopt;
    }

    if (columnRef->columnName != pkColumn ||
        (!columnRef->tableName.empty() && columnRef->tableName != tableName)) {
        return std::nullopt;
    }
    if (literal->literalType != LiteralType::INTEGER) {
        return std::nullopt;
    }

    int32_t value = static_cast<int32_t>(std::stol(literal->value));
    BinaryOperator op = binary->op;
    if (!columnOnLeft) {
        switch (op) {
            case BinaryOperator::LESS_THAN: op = BinaryOperator::GREATER_THAN; break;
            case BinaryOperator::GREATER_THAN: op = BinaryOperator::LESS_THAN; break;
            case BinaryOperator::LESS_EQUAL: op = BinaryOperator::GREATER_EQUAL; break;
            case BinaryOperator::GREATER_EQUAL: op = BinaryOperator::LESS_EQUAL; break;
            default: break;
        }
    }

    constexpr int32_t kMin = std::numeric_limits<int32_t>::min();
    constexpr int32_t kMax = std::numeric_limits<int32_t>::max();

    switch (op) {
        case BinaryOperator::EQUALS: return std::make_pair(value, value);
        case BinaryOperator::GREATER_EQUAL: return std::make_pair(value, kMax);
        case BinaryOperator::GREATER_THAN: return std::make_pair(value + 1, kMax);
        case BinaryOperator::LESS_EQUAL: return std::make_pair(kMin, value);
        case BinaryOperator::LESS_THAN: return std::make_pair(kMin, value - 1);
        default: return std::nullopt;
    }
}

ResultSet Executor::ExecuteSelect(const SelectStatement* select) {
    if (select->join) {
        return ExecuteJoinSelect(select);
    }
    return ExecuteSingleTableSelect(select);
}

ResultSet Executor::ExecuteSingleTableSelect(const SelectStatement* select) {
    const Schema& schema = storageEngine_.GetSchema(select->tableName);
    std::vector<Row> candidateRows;
    bool usedIndex = false;

    if (select->whereClause) {
        BTree* index = GetOrLoadIndex(select->tableName);
        if (index != nullptr) {
            const std::string& pkColumn = primaryKeyColumnNames_[select->tableName];
            std::optional<std::pair<int32_t, int32_t>> range =
                TryExtractIndexRange(select->whereClause.get(), select->tableName, pkColumn);
            if (range) {
                usedIndex = true;
                if (range->first == range->second) {
                    std::optional<RowLocation> location = index->Search(range->first);
                    if (location) {
                        candidateRows.push_back(storageEngine_.ReadRow(select->tableName, *location));
                    }
                } else if (range->first <= range->second) {
                    auto matches = index->RangeSearch(range->first, range->second);
                    candidateRows.reserve(matches.size());
                    for (const auto& [key, location] : matches) {
                        candidateRows.push_back(storageEngine_.ReadRow(select->tableName, location));
                    }
                }
            }
        }
    }

    if (!usedIndex) {
        candidateRows = storageEngine_.ReadAllRows(select->tableName);
    }

    ResultSet result;
    result.usedIndex = usedIndex;
    result.rowsExamined = candidateRows.size();

    std::vector<int> projectionIndices;
    if (select->selectAll) {
        for (size_t i = 0; i < schema.GetColumns().size(); i++) {
            projectionIndices.push_back(static_cast<int>(i));
            result.columnNames.push_back(schema.GetColumns()[i].name);
        }
    } else {
        for (const std::string& colName : select->columns) {
            std::string bareName = colName;
            size_t dotPos = colName.find('.');
            if (dotPos != std::string::npos) {
                bareName = colName.substr(dotPos + 1);
            }
            int idx = schema.FindColumnIndex(bareName);
            if (idx < 0) {
                throw MiniSqlException("Unknown column '" + colName + "' in SELECT list");
            }
            projectionIndices.push_back(idx);
            result.columnNames.push_back(colName);
        }
    }

    for (const Row& row : candidateRows) {
        if (select->whereClause) {
            RowContext context;
            context.Add(select->tableName, schema, row);
            ExpressionEvaluator evaluator(context);
            if (!evaluator.EvaluateBoolean(select->whereClause.get())) {
                continue;
            }
        }

        std::vector<Value> cells;
        cells.reserve(projectionIndices.size());
        for (int idx : projectionIndices) {
            cells.push_back(row.GetValue(static_cast<size_t>(idx)));
        }
        result.rows.push_back(std::move(cells));
    }

    return result;
}

ResultSet Executor::ExecuteJoinSelect(const SelectStatement* select) {
    const Schema& leftSchema = storageEngine_.GetSchema(select->tableName);
    const Schema& rightSchema = storageEngine_.GetSchema(select->join->tableName);

    bool leftMatchesOnTable = (select->join->leftTable == select->tableName);
    const std::string& leftJoinColumn =
        leftMatchesOnTable ? select->join->leftColumn : select->join->rightColumn;
    const std::string& rightJoinColumn =
        leftMatchesOnTable ? select->join->rightColumn : select->join->leftColumn;

    int leftJoinIndex = leftSchema.FindColumnIndex(leftJoinColumn);
    int rightJoinIndex = rightSchema.FindColumnIndex(rightJoinColumn);
    if (leftJoinIndex < 0 || rightJoinIndex < 0) {
        throw MiniSqlException("JOIN ON clause references an unknown column");
    }
    if (leftSchema.GetColumns()[leftJoinIndex].type != rightSchema.GetColumns()[rightJoinIndex].type) {
        throw MiniSqlException("JOIN columns must have matching types");
    }

    std::vector<std::pair<bool, int>> projection;
    ResultSet result;

    if (select->selectAll) {
        for (size_t i = 0; i < leftSchema.GetColumns().size(); i++) {
            projection.push_back({true, static_cast<int>(i)});
            result.columnNames.push_back(select->tableName + "." + leftSchema.GetColumns()[i].name);
        }
        for (size_t i = 0; i < rightSchema.GetColumns().size(); i++) {
            projection.push_back({false, static_cast<int>(i)});
            result.columnNames.push_back(select->join->tableName + "." + rightSchema.GetColumns()[i].name);
        }
    } else {
        for (const std::string& raw : select->columns) {
            std::string tablePart, columnPart = raw;
            size_t dotPos = raw.find('.');
            if (dotPos != std::string::npos) {
                tablePart = raw.substr(0, dotPos);
                columnPart = raw.substr(dotPos + 1);
            }

            if (!tablePart.empty()) {
                if (tablePart == select->tableName) {
                    int idx = leftSchema.FindColumnIndex(columnPart);
                    if (idx < 0) throw MiniSqlException("Unknown column '" + raw + "'");
                    projection.push_back({true, idx});
                } else if (tablePart == select->join->tableName) {
                    int idx = rightSchema.FindColumnIndex(columnPart);
                    if (idx < 0) throw MiniSqlException("Unknown column '" + raw + "'");
                    projection.push_back({false, idx});
                } else {
                    throw MiniSqlException("Unknown table qualifier '" + tablePart + "' in SELECT list");
                }
            } else {
                int leftIdx = leftSchema.FindColumnIndex(columnPart);
                int rightIdx = rightSchema.FindColumnIndex(columnPart);
                if (leftIdx >= 0 && rightIdx >= 0) {
                    throw MiniSqlException("Ambiguous column '" + columnPart +
                                            "' in JOIN, qualify it with a table name");
                }
                if (leftIdx >= 0) {
                    projection.push_back({true, leftIdx});
                } else if (rightIdx >= 0) {
                    projection.push_back({false, rightIdx});
                } else {
                    throw MiniSqlException("Unknown column '" + raw + "' in SELECT list");
                }
            }
            result.columnNames.push_back(raw);
        }
    }

    std::vector<Row> leftRows = storageEngine_.ReadAllRows(select->tableName);
    std::vector<Row> rightRows = storageEngine_.ReadAllRows(select->join->tableName);
    result.rowsExamined = leftRows.size() * rightRows.size();

    for (const Row& leftRow : leftRows) {
        for (const Row& rightRow : rightRows) {
            if (!(leftRow.GetValue(static_cast<size_t>(leftJoinIndex)) ==
                  rightRow.GetValue(static_cast<size_t>(rightJoinIndex)))) {
                continue;
            }

            if (select->whereClause) {
                RowContext context;
                context.Add(select->tableName, leftSchema, leftRow);
                context.Add(select->join->tableName, rightSchema, rightRow);
                ExpressionEvaluator evaluator(context);
                if (!evaluator.EvaluateBoolean(select->whereClause.get())) {
                    continue;
                }
            }

            std::vector<Value> cells;
            cells.reserve(projection.size());
            for (const auto& [isLeft, idx] : projection) {
                cells.push_back(isLeft ? leftRow.GetValue(static_cast<size_t>(idx))
                                        : rightRow.GetValue(static_cast<size_t>(idx)));
            }
            result.rows.push_back(std::move(cells));
        }
    }

    return result;
}

}  // namespace minisql
