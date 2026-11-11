#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "storage/row.h"
#include "storage/schema.h"
#include "storage/table_storage.h"

namespace minisql {

class StorageEngine {
public:
    explicit StorageEngine(std::string dataDirectory);

    void CreateTable(const Schema& schema);
    RowLocation InsertRow(const std::string& tableName, const Row& row);
    Row ReadRow(const std::string& tableName, const RowLocation& location);
    std::vector<Row> ReadAllRows(const std::string& tableName);
    const Schema& GetSchema(const std::string& tableName);
    bool TableExists(const std::string& tableName) const;

private:
    std::string dataDirectory_;
    std::map<std::string, std::unique_ptr<TableStorage>> openTables_;

    std::string GetTableFilePath(const std::string& tableName) const;
    TableStorage& GetOrOpenTable(const std::string& tableName);
};

}  // namespace minisql
