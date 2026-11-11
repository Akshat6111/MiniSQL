#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "btree/btree_node.h"
#include "storage/page_manager.h"
#include "storage/row_location.h"

namespace minisql {

class BTree {
public:
    static std::unique_ptr<BTree> Create(const std::string& filePath);
    static std::unique_ptr<BTree> Open(const std::string& filePath);

    void Insert(int32_t key, const RowLocation& location);
    std::optional<RowLocation> Search(int32_t key) const;
    std::vector<std::pair<int32_t, RowLocation>> RangeSearch(int32_t lowKey, int32_t highKey) const;

    size_t GetHeight() const;
    size_t GetNodeCount() const;

private:
    BTree(std::unique_ptr<PageManager> pageManager, uint32_t rootPageId);

    std::unique_ptr<PageManager> pageManager_;
    uint32_t rootPageId_;

    BTreeNode ReadNode(uint32_t pageId) const;
    void WriteNode(const BTreeNode& node);
    void WriteMetadata() const;

    std::optional<RowLocation> SearchNode(uint32_t pageId, int32_t key) const;
    void RangeSearchNode(uint32_t pageId, int32_t low, int32_t high,
                          std::vector<std::pair<int32_t, RowLocation>>& results) const;

    void InsertNonFull(BTreeNode& node, int32_t key, const RowLocation& value);
    void SplitChild(BTreeNode& parent, size_t childIndex, BTreeNode& child);
};

}  // namespace minisql
