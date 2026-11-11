#include "btree/btree.h"

#include "common/exceptions.h"

namespace minisql {

namespace {
constexpr uint32_t kMetadataPageId = 0;
}  // namespace

BTree::BTree(std::unique_ptr<PageManager> pageManager, uint32_t rootPageId)
    : pageManager_(std::move(pageManager)), rootPageId_(rootPageId) {}

std::unique_ptr<BTree> BTree::Create(const std::string& filePath) {
    auto pageManager = std::make_unique<PageManager>(filePath);
    if (pageManager->GetPageCount() != 0) {
        throw MiniSqlException("Cannot create index: file already contains data: " + filePath);
    }

    uint32_t metadataPageId = pageManager->AllocatePage();
    (void)metadataPageId;  // always 0 by construction

    uint32_t rootPageId = pageManager->AllocatePage();
    BTreeNode rootNode;
    rootNode.pageId = rootPageId;
    rootNode.isLeaf = true;

    Page rootPage(rootPageId);
    rootNode.SerializeTo(rootPage);
    pageManager->WritePage(rootPage);

    Page metadataPage(kMetadataPageId);
    metadataPage.WriteUint32(0, rootPageId);
    pageManager->WritePage(metadataPage);

    return std::unique_ptr<BTree>(new BTree(std::move(pageManager), rootPageId));
}

std::unique_ptr<BTree> BTree::Open(const std::string& filePath) {
    auto pageManager = std::make_unique<PageManager>(filePath);
    if (pageManager->GetPageCount() < 2) {
        throw MiniSqlException("Cannot open index: file is missing metadata or root page: " + filePath);
    }

    Page metadataPage(kMetadataPageId);
    pageManager->ReadPage(kMetadataPageId, metadataPage);
    uint32_t rootPageId = metadataPage.ReadUint32(0);

    return std::unique_ptr<BTree>(new BTree(std::move(pageManager), rootPageId));
}

BTreeNode BTree::ReadNode(uint32_t pageId) const {
    Page page(pageId);
    pageManager_->ReadPage(pageId, page);
    return BTreeNode::DeserializeFrom(page);
}

void BTree::WriteNode(const BTreeNode& node) {
    Page page(node.pageId);
    node.SerializeTo(page);
    pageManager_->WritePage(page);
}

void BTree::WriteMetadata() const {
    Page metadataPage(kMetadataPageId);
    metadataPage.WriteUint32(0, rootPageId_);
    pageManager_->WritePage(metadataPage);
}

void BTree::SplitChild(BTreeNode& parent, size_t childIndex, BTreeNode& child) {
    size_t mid = kBTreeMaxKeys / 2;
    int32_t medianKey = child.keys[mid];
    RowLocation medianValue = child.values[mid];

    BTreeNode rightSibling;
    rightSibling.pageId = pageManager_->AllocatePage();
    rightSibling.isLeaf = child.isLeaf;
    rightSibling.keys.assign(child.keys.begin() + mid + 1, child.keys.end());
    rightSibling.values.assign(child.values.begin() + mid + 1, child.values.end());
    if (!child.isLeaf) {
        rightSibling.children.assign(child.children.begin() + mid + 1, child.children.end());
        child.children.resize(mid + 1);
    }
    child.keys.resize(mid);
    child.values.resize(mid);

    parent.keys.insert(parent.keys.begin() + childIndex, medianKey);
    parent.values.insert(parent.values.begin() + childIndex, medianValue);
    parent.children.insert(parent.children.begin() + childIndex + 1, rightSibling.pageId);

    WriteNode(child);
    WriteNode(rightSibling);
}

void BTree::Insert(int32_t key, const RowLocation& location) {
    BTreeNode root = ReadNode(rootPageId_);

    if (root.keys.size() == kBTreeMaxKeys) {
        BTreeNode newRoot;
        newRoot.pageId = pageManager_->AllocatePage();
        newRoot.isLeaf = false;
        newRoot.children.push_back(rootPageId_);

        SplitChild(newRoot, 0, root);
        WriteNode(newRoot);

        rootPageId_ = newRoot.pageId;
        WriteMetadata();

        InsertNonFull(newRoot, key, location);
    } else {
        InsertNonFull(root, key, location);
    }
}

void BTree::InsertNonFull(BTreeNode& node, int32_t key, const RowLocation& value) {
    size_t i = 0;
    while (i < node.keys.size() && key > node.keys[i]) {
        i++;
    }
    if (i < node.keys.size() && node.keys[i] == key) {
        throw MiniSqlException("Duplicate key in index: " + std::to_string(key));
    }

    if (node.isLeaf) {
        node.keys.insert(node.keys.begin() + i, key);
        node.values.insert(node.values.begin() + i, value);
        WriteNode(node);
        return;
    }

    BTreeNode child = ReadNode(node.children[i]);
    if (child.keys.size() == kBTreeMaxKeys) {
        SplitChild(node, i, child);
        WriteNode(node);

        if (key > node.keys[i]) {
            i++;
        } else if (key == node.keys[i]) {
            throw MiniSqlException("Duplicate key in index: " + std::to_string(key));
        }
        child = ReadNode(node.children[i]);
    }

    InsertNonFull(child, key, value);
}

std::optional<RowLocation> BTree::Search(int32_t key) const {
    return SearchNode(rootPageId_, key);
}

std::optional<RowLocation> BTree::SearchNode(uint32_t pageId, int32_t key) const {
    BTreeNode node = ReadNode(pageId);

    size_t i = 0;
    while (i < node.keys.size() && key > node.keys[i]) {
        i++;
    }
    if (i < node.keys.size() && node.keys[i] == key) {
        return node.values[i];
    }
    if (node.isLeaf) {
        return std::nullopt;
    }
    return SearchNode(node.children[i], key);
}

std::vector<std::pair<int32_t, RowLocation>> BTree::RangeSearch(int32_t lowKey, int32_t highKey) const {
    std::vector<std::pair<int32_t, RowLocation>> results;
    if (lowKey > highKey) {
        return results;
    }
    RangeSearchNode(rootPageId_, lowKey, highKey, results);
    return results;
}

void BTree::RangeSearchNode(uint32_t pageId, int32_t low, int32_t high,
                             std::vector<std::pair<int32_t, RowLocation>>& results) const {
    BTreeNode node = ReadNode(pageId);
    size_t n = node.keys.size();

    for (size_t i = 0; i < n; i++) {
        bool leftChildMayContainRange =
            !node.isLeaf && (i == 0 || node.keys[i - 1] < high) && node.keys[i] > low;
        if (leftChildMayContainRange) {
            RangeSearchNode(node.children[i], low, high, results);
        }

        if (node.keys[i] >= low && node.keys[i] <= high) {
            results.emplace_back(node.keys[i], node.values[i]);
        }

        if (node.keys[i] > high) {
            return;
        }
    }

    bool rightChildMayContainRange = !node.isLeaf && (n == 0 || node.keys[n - 1] < high);
    if (rightChildMayContainRange) {
        RangeSearchNode(node.children[n], low, high, results);
    }
}

size_t BTree::GetHeight() const {
    size_t height = 1;
    BTreeNode node = ReadNode(rootPageId_);
    while (!node.isLeaf) {
        node = ReadNode(node.children[0]);
        height++;
    }
    return height;
}

size_t BTree::GetNodeCount() const {
    return pageManager_->GetPageCount() - 1;  // exclude the metadata page
}

}  // namespace minisql
