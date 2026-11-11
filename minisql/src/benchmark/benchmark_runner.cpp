#include "benchmark/benchmark_runner.h"

#include <algorithm>
#include <utility>

#include "benchmark/timer.h"

namespace minisql {

BenchmarkRunner::BenchmarkRunner(StorageEngine& storage, BTree& index, std::string tableName,
                                  size_t datasetSize)
    : storage_(storage), index_(index), tableName_(std::move(tableName)), datasetSize_(datasetSize) {}

std::optional<Row> BenchmarkRunner::FullScanLookup(int32_t key) {
    std::vector<Row> rows = storage_.ReadAllRows(tableName_);
    for (Row& row : rows) {
        if (std::get<int32_t>(row.GetValue(0)) == key) {
            return row;
        }
    }
    return std::nullopt;
}

std::optional<Row> BenchmarkRunner::IndexedLookup(int32_t key) {
    std::optional<RowLocation> location = index_.Search(key);
    if (!location) {
        return std::nullopt;
    }
    return storage_.ReadRow(tableName_, *location);
}

LookupBenchmarkResult BenchmarkRunner::RunPointLookupBenchmark(const std::vector<int32_t>& keys) {
    Stopwatch fullScanTimer;
    for (int32_t key : keys) {
        std::optional<Row> row = FullScanLookup(key);
        (void)row;
    }
    double fullScanTotalMs = fullScanTimer.ElapsedMilliseconds();

    Stopwatch indexedTimer;
    for (int32_t key : keys) {
        std::optional<Row> row = IndexedLookup(key);
        (void)row;
    }
    double indexedTotalMs = indexedTimer.ElapsedMilliseconds();

    size_t numQueries = keys.size();
    double fullScanAvgMicros = (fullScanTotalMs * 1000.0) / static_cast<double>(numQueries);
    double indexedAvgMicros = (indexedTotalMs * 1000.0) / static_cast<double>(numQueries);
    double speedup = indexedAvgMicros > 0.0 ? fullScanAvgMicros / indexedAvgMicros : 0.0;

    return LookupBenchmarkResult{datasetSize_,
                                  numQueries,
                                  fullScanTotalMs,
                                  fullScanAvgMicros,
                                  datasetSize_,
                                  indexedTotalMs,
                                  indexedAvgMicros,
                                  1,
                                  speedup};
}

RangeBenchmarkResult BenchmarkRunner::RunRangeBenchmark(int32_t lowKey, int32_t highKey) {
    Stopwatch fullScanTimer;
    std::vector<Row> allRows = storage_.ReadAllRows(tableName_);
    size_t scanMatchCount = 0;
    for (const Row& row : allRows) {
        int32_t id = std::get<int32_t>(row.GetValue(0));
        if (id >= lowKey && id <= highKey) {
            scanMatchCount++;
        }
    }
    double fullScanMs = fullScanTimer.ElapsedMilliseconds();

    Stopwatch indexedTimer;
    std::vector<std::pair<int32_t, RowLocation>> matches = index_.RangeSearch(lowKey, highKey);
    for (const auto& [key, location] : matches) {
        storage_.ReadRow(tableName_, location);
    }
    double indexedMs = indexedTimer.ElapsedMilliseconds();

    double speedup = indexedMs > 0.0 ? fullScanMs / indexedMs : 0.0;
    size_t rangeWidth = static_cast<size_t>(highKey - lowKey + 1);

    return RangeBenchmarkResult{datasetSize_, rangeWidth, scanMatchCount, fullScanMs, indexedMs, speedup};
}

}  // namespace minisql
