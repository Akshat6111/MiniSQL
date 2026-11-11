#include <algorithm>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <random>
#include <vector>

#include "btree/btree.h"
#include "common/exceptions.h"
#include "storage/storage_engine.h"

using namespace minisql;

int main() {
    std::string dataDir = "/tmp/minisql_btree_demo";
    std::filesystem::remove_all(dataDir);
    std::filesystem::create_directories(dataDir);

    Schema schema("users", {
                                Column{"id", ColumnType::INT, true},
                                Column{"name", ColumnType::VARCHAR, false},
                            });

    const int kRowCount = 5000;

    StorageEngine storage(dataDir);
    storage.CreateTable(schema);

    std::vector<int32_t> keys(kRowCount);
    for (int i = 0; i < kRowCount; i++) {
        keys[i] = i;
    }
    std::mt19937 rng(42);
    std::shuffle(keys.begin(), keys.end(), rng);

    std::string indexPath = dataDir + "/users_id.idx";
    std::unique_ptr<BTree> index = BTree::Create(indexPath);

    for (int32_t key : keys) {
        Row row({Value(key), Value(std::string("user_") + std::to_string(key))});
        RowLocation location = storage.InsertRow("users", row);
        index->Insert(key, location);
    }

    std::cout << "Inserted " << kRowCount << " rows in shuffled order.\n";
    std::cout << "B-Tree height:     " << index->GetHeight() << "\n";
    std::cout << "B-Tree node count: " << index->GetNodeCount() << "\n\n";

    std::cout << "--- Point lookups ---\n";
    for (int32_t key : {0, 1, 2500, 4999, 17}) {
        std::optional<RowLocation> location = index->Search(key);
        assert(location.has_value());
        Row row = storage.ReadRow("users", *location);
        assert(std::get<int32_t>(row.GetValue(0)) == key);
        assert(std::get<std::string>(row.GetValue(1)) == "user_" + std::to_string(key));
        std::cout << "  Search(" << key << ") -> page " << location->pageId << " slot "
                   << location->slotIndex << " -> row id=" << std::get<int32_t>(row.GetValue(0))
                   << " name=" << std::get<std::string>(row.GetValue(1)) << "\n";
    }

    std::optional<RowLocation> missing = index->Search(999999);
    assert(!missing.has_value());
    std::cout << "  Search(999999) -> not found, as expected\n\n";

    std::cout << "--- Range query [1000, 1010] ---\n";
    std::vector<std::pair<int32_t, RowLocation>> range = index->RangeSearch(1000, 1010);
    assert(range.size() == 11);
    for (size_t i = 0; i < range.size(); i++) {
        assert(range[i].first == 1000 + static_cast<int32_t>(i));
    }
    for (const auto& [key, location] : range) {
        std::cout << "  " << key;
    }
    std::cout << "\n  Count: " << range.size() << " (expected 11, sorted ascending) - verified\n\n";

    std::cout << "--- Duplicate key rejection ---\n";
    try {
        index->Insert(42, RowLocation{0, 0});
        std::cout << "  ERROR: expected duplicate key to throw\n";
    } catch (const MiniSqlException& e) {
        std::cout << "  Correctly rejected duplicate key: " << e.what() << "\n\n";
    }

    std::cout << "--- Persistence across reopening the index file ---\n";
    index.reset();
    std::unique_ptr<BTree> reopened = BTree::Open(indexPath);
    for (int32_t key : {3333, 42, 4999}) {
        std::optional<RowLocation> location = reopened->Search(key);
        assert(location.has_value());
    }
    std::cout << "  Reopened index and re-verified 3 lookups successfully.\n";
    std::cout << "  Height after reopen: " << reopened->GetHeight()
               << ", node count: " << reopened->GetNodeCount() << "\n\n";

    std::cout << "All B-Tree checks passed.\n";
    return 0;
}
