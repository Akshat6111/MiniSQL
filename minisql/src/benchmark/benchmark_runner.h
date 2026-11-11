#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "btree/btree.h"
#include "storage/storage_engine.h"

namespace minisql {

struct LookupBenchmarkResult {
    size_t datasetSize;
    size_t numQueries;
    double fullScanTotalMs;
    double fullScanAvgMicros;
    size_t fullScanRowsExaminedPerQuery;
    double indexedTotalMs;
    double indexedAvgMicros;
    size_t indexedRowsExaminedPerQuery;
    double speedupFactor;
};

struct RangeBenchmarkResult {
    size_t datasetSize;
    size_t rangeWidth;
    size_t matchCount;
    double fullScanMs;
    double indexedMs;
    double speedupFactor;
};

class BenchmarkRunner {
public:
    BenchmarkRunner(StorageEngine& storage, BTree& index, std::string tableName, size_t datasetSize);

    LookupBenchmarkResult RunPointLookupBenchmark(const std::vector<int32_t>& keys);
    RangeBenchmarkResult RunRangeBenchmark(int32_t lowKey, int32_t highKey);

private:
    StorageEngine& storage_;
    BTree& index_;
    std::string tableName_;
    size_t datasetSize_;

    std::optional<Row> FullScanLookup(int32_t key);
    std::optional<Row> IndexedLookup(int32_t key);
};

}  // namespace minisql
