#pragma once

#include <cstdint>

namespace minisql {

struct RowLocation {
    uint32_t pageId;
    uint32_t slotIndex;
};

}  // namespace minisql
