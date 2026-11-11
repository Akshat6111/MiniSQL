#include "storage/storage_engine.h"

#include <filesystem>

#include "common/exceptions.h"

namespace minisql {

StorageEngine::StorageEngine(std::string dataDirectory) : dataDirectory_(std::move(dataDirectory)) {
    std::filesystem::create_directories(dataDirectory_);
}

std::string StorageEngine::GetTableFilePath(const std::string& tableName) const {
    return dataDirectory_ + "/" + tableName + ".db";
}

bool StorageEngine::TableExists(const std::string& tableName) const {
    if (openTables_.find(tableName) != openTables_.end()) {
        return true;
    }
    return std::filesystem::exists(GetTableFilePath(tableName));
}

void StorageEngine::CreateTable(const Schema& schema) {
    const std::string& tableName = schema.GetTableName();
    if (TableExists(tableName)) {
        throw MiniSqlException("Table '" + tableName + "' already exists");
    }

    std::unique_ptr<TableStorage> table = TableStorage::Create(GetTableFilePath(tableName), schema);
    openTables_[tableName] = std::move(table);
}

TableStorage& StorageEngine::GetOrOpenTable(const std::string& tableName) {
    auto it = openTables_.find(tableName);
    if (it != openTables_.end()) {
        return *it->second;
    }

    std::string filePath = GetTableFilePath(tableName);
    if (!std::filesystem::exists(filePath)) {
        throw MiniSqlException("Table '" + tableName + "' does not exist");
    }

    std::unique_ptr<TableStorage> table = TableStorage::Open(filePath);
    TableStorage& tableRef = *table;
    openTables_[tableName] = std::move(table);
    return tableRef;
}

RowLocation StorageEngine::InsertRow(const std::string& tableName, const Row& row) {
    return GetOrOpenTable(tableName).InsertRow(row);
}

Row StorageEngine::ReadRow(const std::string& tableName, const RowLocation& location) {
    return GetOrOpenTable(tableName).ReadRow(location);
}

std::vector<Row> StorageEngine::ReadAllRows(const std::string& tableName) {
    return GetOrOpenTable(tableName).ReadAllRows();
}

const Schema& StorageEngine::GetSchema(const std::string& tableName) {
    return GetOrOpenTable(tableName).GetSchema();
}

}  // namespace minisql
