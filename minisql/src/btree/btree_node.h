#pragma once

#include <cstdint>
#include <vector>

#include "storage/page.h"
#include "storage/row_location.h"

namespace minisql {

constexpr size_t kBTreeNodeHeaderSize = 8;
constexpr size_t kBTreeEntrySize = sizeof(int32_t) + 2 * sizeof(uint32_t);  // key + RowLocation
constexpr size_t kBTreeMaxKeys =
    (kPageSize - kBTreeNodeHeaderSize - sizeof(uint32_t)) / (kBTreeEntrySize + sizeof(uint32_t));
constexpr size_t kBTreeMinDegree = (kBTreeMaxKeys + 1) / 2;
constexpr size_t kBTreeMinKeys = kBTreeMinDegree - 1;

struct BTreeNode {
    uint32_t pageId = 0;
    bool isLeaf = true;
    std::vector<int32_t> keys;
    std::vector<RowLocation> values;
    std::vector<uint32_t> children;

    void SerializeTo(Page& page) const;
    static BTreeNode DeserializeFrom(const Page& page);
};

}  // namespace minisql
