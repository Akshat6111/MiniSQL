#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "btree/btree.h"
#include "storage/storage_engine.h"

namespace minisql {

struct GenerationStats {
    size_t rowCount;
    double totalTimeMs;
    double rowsPerSecond;
};

class DatasetGenerator {
public:
    DatasetGenerator(StorageEngine& storage, std::string dataDirectory);

    GenerationStats Generate(const std::string& tableName, size_t rowCount);
    BTree& GetIndex();

private:
    StorageEngine& storage_;
    std::string dataDirectory_;
    std::unique_ptr<BTree> index_;
};

}  // namespace minisql
