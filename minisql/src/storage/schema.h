#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "storage/page.h"

namespace minisql {

constexpr size_t kVarcharMaxLength = 64;

enum class ColumnType : uint8_t { INT = 0, VARCHAR = 1 };

struct Column {
    std::string name;
    ColumnType type;
    bool isPrimaryKey;

    size_t SerializedSize() const;
};

class Schema {
public:
    Schema() = default;
    Schema(std::string tableName, std::vector<Column> columns);

    const std::string& GetTableName() const;
    const std::vector<Column>& GetColumns() const;
    size_t GetRowSize() const;
    int FindColumnIndex(const std::string& columnName) const;

    void WriteToPage(Page& page) const;
    static Schema ReadFromPage(const Page& page);

private:
    std::string tableName_;
    std::vector<Column> columns_;
};

}  // namespace minisql
