#include "storage/schema.h"

#include <utility>

#include "common/exceptions.h"

namespace minisql {

size_t Column::SerializedSize() const {
    return type == ColumnType::INT ? sizeof(int32_t) : kVarcharMaxLength;
}

Schema::Schema(std::string tableName, std::vector<Column> columns)
    : tableName_(std::move(tableName)), columns_(std::move(columns)) {}

const std::string& Schema::GetTableName() const {
    return tableName_;
}

const std::vector<Column>& Schema::GetColumns() const {
    return columns_;
}

size_t Schema::GetRowSize() const {
    size_t total = 0;
    for (const Column& column : columns_) {
        total += column.SerializedSize();
    }
    return total;
}

int Schema::FindColumnIndex(const std::string& columnName) const {
    for (size_t i = 0; i < columns_.size(); i++) {
        if (columns_[i].name == columnName) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void Schema::WriteToPage(Page& page) const {
    size_t offset = 0;

    page.WriteUint32(offset, static_cast<uint32_t>(tableName_.size()));
    offset += sizeof(uint32_t);
    page.WriteBytes(offset, reinterpret_cast<const uint8_t*>(tableName_.data()), tableName_.size());
    offset += tableName_.size();

    page.WriteUint32(offset, static_cast<uint32_t>(columns_.size()));
    offset += sizeof(uint32_t);

    for (const Column& column : columns_) {
        page.WriteUint32(offset, static_cast<uint32_t>(column.name.size()));
        offset += sizeof(uint32_t);
        page.WriteBytes(offset, reinterpret_cast<const uint8_t*>(column.name.data()), column.name.size());
        offset += column.name.size();

        page.WriteUint32(offset, static_cast<uint32_t>(column.type));
        offset += sizeof(uint32_t);

        page.WriteUint32(offset, column.isPrimaryKey ? 1 : 0);
        offset += sizeof(uint32_t);
    }
}

Schema Schema::ReadFromPage(const Page& page) {
    size_t offset = 0;

    uint32_t tableNameLength = page.ReadUint32(offset);
    offset += sizeof(uint32_t);

    std::vector<uint8_t> nameBuffer(tableNameLength);
    page.ReadBytes(offset, nameBuffer.data(), tableNameLength);
    offset += tableNameLength;
    std::string tableName(nameBuffer.begin(), nameBuffer.end());

    uint32_t columnCount = page.ReadUint32(offset);
    offset += sizeof(uint32_t);

    std::vector<Column> columns;
    columns.reserve(columnCount);

    for (uint32_t i = 0; i < columnCount; i++) {
        uint32_t columnNameLength = page.ReadUint32(offset);
        offset += sizeof(uint32_t);

        std::vector<uint8_t> columnNameBuffer(columnNameLength);
        page.ReadBytes(offset, columnNameBuffer.data(), columnNameLength);
        offset += columnNameLength;
        std::string columnName(columnNameBuffer.begin(), columnNameBuffer.end());

        uint32_t typeValue = page.ReadUint32(offset);
        offset += sizeof(uint32_t);
        if (typeValue != static_cast<uint32_t>(ColumnType::INT) &&
            typeValue != static_cast<uint32_t>(ColumnType::VARCHAR)) {
            throw MiniSqlException("Corrupt catalog page: unknown column type " +
                                    std::to_string(typeValue));
        }

        uint32_t isPrimaryKeyValue = page.ReadUint32(offset);
        offset += sizeof(uint32_t);

        columns.push_back(Column{columnName, static_cast<ColumnType>(typeValue), isPrimaryKeyValue != 0});
    }

    return Schema(tableName, columns);
}

}  // namespace minisql
