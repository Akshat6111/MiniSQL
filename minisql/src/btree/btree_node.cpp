#include "btree/btree_node.h"

namespace minisql {

void BTreeNode::SerializeTo(Page& page) const {
    page.WriteUint32(0, isLeaf ? 1 : 0);
    page.WriteUint32(4, static_cast<uint32_t>(keys.size()));

    size_t offset = kBTreeNodeHeaderSize;
    for (size_t i = 0; i < keys.size(); i++) {
        page.WriteInt32(offset, keys[i]);
        offset += sizeof(int32_t);
        page.WriteUint32(offset, values[i].pageId);
        offset += sizeof(uint32_t);
        page.WriteUint32(offset, values[i].slotIndex);
        offset += sizeof(uint32_t);
    }

    if (!isLeaf) {
        for (uint32_t childPageId : children) {
            page.WriteUint32(offset, childPageId);
            offset += sizeof(uint32_t);
        }
    }
}

BTreeNode BTreeNode::DeserializeFrom(const Page& page) {
    BTreeNode node;
    node.pageId = page.GetPageId();
    node.isLeaf = page.ReadUint32(0) != 0;
    uint32_t keyCount = page.ReadUint32(4);

    size_t offset = kBTreeNodeHeaderSize;
    node.keys.reserve(keyCount);
    node.values.reserve(keyCount);
    for (uint32_t i = 0; i < keyCount; i++) {
        int32_t key = page.ReadInt32(offset);
        offset += sizeof(int32_t);
        uint32_t rowPageId = page.ReadUint32(offset);
        offset += sizeof(uint32_t);
        uint32_t slotIndex = page.ReadUint32(offset);
        offset += sizeof(uint32_t);
        node.keys.push_back(key);
        node.values.push_back(RowLocation{rowPageId, slotIndex});
    }

    if (!node.isLeaf) {
        node.children.reserve(keyCount + 1);
        for (uint32_t i = 0; i <= keyCount; i++) {
            node.children.push_back(page.ReadUint32(offset));
            offset += sizeof(uint32_t);
        }
    }

    return node;
}

}  // namespace minisql
