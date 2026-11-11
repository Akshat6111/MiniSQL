#pragma once

#include <string>
#include <vector>

#include "storage/row.h"

namespace minisql {

struct ResultSet {
    std::vector<std::string> columnNames;
    std::vector<std::vector<Value>> rows;
    bool usedIndex = false;
    size_t rowsExamined = 0;
};

}  // namespace minisql
