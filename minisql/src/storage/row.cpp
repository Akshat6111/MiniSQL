#include "storage/row.h"

#include <sstream>
#include <utility>

#include "common/exceptions.h"

namespace minisql {

Row::Row(std::vector<Value> values) : values_(std::move(values)) {}

const std::vector<Value>& Row::GetValues() const {
    return values_;
}

const Value& Row::GetValue(size_t index) const {
    if (index >= values_.size()) {
        throw MiniSqlException("Row column index out of range: " + std::to_string(index));
    }
    return values_[index];
}

size_t Row::ColumnCount() const {
    return values_.size();
}

void Row::SerializeTo(Page& page, size_t offset, const Schema& schema) const {
    const std::vector<Column>& columns = schema.GetColumns();
    if (values_.size() != columns.size()) {
        throw MiniSqlException("Row has " + std::to_string(values_.size()) + " values but schema for '" +
                                schema.GetTableName() + "' expects " + std::to_string(columns.size()));
    }

    size_t currentOffset = offset;
    for (size_t i = 0; i < columns.size(); i++) {
        const Column& column = columns[i];
        if (column.type == ColumnType::INT) {
            if (!std::holds_alternative<int32_t>(values_[i])) {
                throw MiniSqlException("Expected INT value for column '" + column.name + "'");
            }
            page.WriteInt32(currentOffset, std::get<int32_t>(values_[i]));
        } else {
            if (!std::holds_alternative<std::string>(values_[i])) {
                throw MiniSqlException("Expected VARCHAR value for column '" + column.name + "'");
            }
            page.WriteString(currentOffset, std::get<std::string>(values_[i]), kVarcharMaxLength);
        }
        currentOffset += column.SerializedSize();
    }
}

Row Row::DeserializeFrom(const Page& page, size_t offset, const Schema& schema) {
    const std::vector<Column>& columns = schema.GetColumns();
    std::vector<Value> values;
    values.reserve(columns.size());

    size_t currentOffset = offset;
    for (const Column& column : columns) {
        if (column.type == ColumnType::INT) {
            values.emplace_back(page.ReadInt32(currentOffset));
        } else {
            values.emplace_back(page.ReadString(currentOffset, kVarcharMaxLength));
        }
        currentOffset += column.SerializedSize();
    }

    return Row(std::move(values));
}

std::string ValueToString(const Value& value) {
    if (std::holds_alternative<int32_t>(value)) {
        return std::to_string(std::get<int32_t>(value));
    }
    return std::get<std::string>(value);
}

}  // namespace minisql
