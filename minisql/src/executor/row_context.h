#pragma once

#include <map>
#include <string>

#include "storage/row.h"
#include "storage/schema.h"

namespace minisql {

class RowContext {
public:
    void Add(const std::string& tableName, const Schema& schema, const Row& row);
    const Value& Resolve(const std::string& tableName, const std::string& columnName) const;

private:
    std::map<std::string, Value> values_;
};

}  // namespace minisql
