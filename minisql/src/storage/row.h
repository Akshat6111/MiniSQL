#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "storage/page.h"
#include "storage/schema.h"

namespace minisql {

using Value = std::variant<int32_t, std::string>;

class Row {
public:
    Row() = default;
    explicit Row(std::vector<Value> values);

    const std::vector<Value>& GetValues() const;
    const Value& GetValue(size_t index) const;
    size_t ColumnCount() const;

    void SerializeTo(Page& page, size_t offset, const Schema& schema) const;
    static Row DeserializeFrom(const Page& page, size_t offset, const Schema& schema);

private:
    std::vector<Value> values_;
};

std::string ValueToString(const Value& value);

}  // namespace minisql
