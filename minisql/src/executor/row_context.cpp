#include "executor/row_context.h"

#include "common/exceptions.h"

namespace minisql {

void RowContext::Add(const std::string& tableName, const Schema& schema, const Row& row) {
    const std::vector<Column>& columns = schema.GetColumns();
    for (size_t i = 0; i < columns.size(); i++) {
        values_[columns[i].name] = row.GetValue(i);
        values_[tableName + "." + columns[i].name] = row.GetValue(i);
    }
}

const Value& RowContext::Resolve(const std::string& tableName, const std::string& columnName) const {
    std::string key = tableName.empty() ? columnName : tableName + "." + columnName;
    auto it = values_.find(key);
    if (it == values_.end()) {
        throw MiniSqlException("Unknown column reference: " + key);
    }
    return it->second;
}

}  // namespace minisql
