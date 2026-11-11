#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

#include "benchmark/benchmark_runner.h"
#include "benchmark/dataset_generator.h"
#include "benchmark/timer.h"
#include "storage/storage_engine.h"

using namespace minisql;

std::vector<int32_t> RandomKeys(size_t count, int32_t maxKeyExclusive, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int32_t> dist(0, maxKeyExclusive - 1);
    std::vector<int32_t> keys(count);
    for (size_t i = 0; i < count; i++) {
        keys[i] = dist(rng);
    }
    return keys;
}

void PrintSeparator() {
    std::cout << std::string(78, '-') << "\n";
}

void RunBenchmarkAtScale(const std::string& dataDir, size_t rowCount) {
    PrintSeparator();
    std::cout << "Dataset size: " << rowCount << " rows\n";
    PrintSeparator();

    std::filesystem::remove_all(dataDir);
    StorageEngine storage(dataDir);

    DatasetGenerator generator(storage, dataDir);
    GenerationStats genStats = generator.Generate("benchmark_users", rowCount);
    std::cout << "Generation:   " << std::fixed << std::setprecision(1) << genStats.totalTimeMs
              << " ms total, " << std::setprecision(0) << genStats.rowsPerSecond << " rows/sec\n\n";

    BenchmarkRunner runner(storage, generator.GetIndex(), "benchmark_users", rowCount);

    size_t numPointQueries = std::min<size_t>(rowCount, 30);
    std::vector<int32_t> keys = RandomKeys(numPointQueries, static_cast<int32_t>(rowCount), 42);
    LookupBenchmarkResult lookup = runner.RunPointLookupBenchmark(keys);

    std::cout << "Point lookup benchmark (" << lookup.numQueries << " random keys):\n";
    std::cout << "  Full table scan:  " << std::setprecision(2) << lookup.fullScanAvgMicros
               << " us/query avg  (" << lookup.fullScanRowsExaminedPerQuery << " rows examined/query, "
               << lookup.fullScanTotalMs << " ms total)\n";
    std::cout << "  B-Tree index:      " << std::setprecision(2) << lookup.indexedAvgMicros
               << " us/query avg  (" << lookup.indexedRowsExaminedPerQuery << " row examined/query, "
               << lookup.indexedTotalMs << " ms total)\n";
    std::cout << "  Speedup:           " << std::setprecision(1) << lookup.speedupFactor << "x faster with index\n\n";

    int32_t rangeWidth = std::max<int32_t>(1, static_cast<int32_t>(rowCount / 100));
    int32_t lowKey = static_cast<int32_t>(rowCount / 3);
    int32_t highKey = lowKey + rangeWidth;
    RangeBenchmarkResult range = runner.RunRangeBenchmark(lowKey, highKey);

    std::cout << "Range query benchmark (" << range.matchCount << " matching rows, ~1% of table):\n";
    std::cout << "  Full table scan:  " << std::setprecision(2) << range.fullScanMs << " ms\n";
    std::cout << "  B-Tree index:      " << std::setprecision(3) << range.indexedMs << " ms\n";
    std::cout << "  Speedup:           " << std::setprecision(1) << range.speedupFactor << "x faster with index\n\n";
}

int main(int argc, char** argv) {
    std::vector<size_t> sizes = {1000, 10000, 100000, 1000000};
    if (argc > 1) {
        sizes.clear();
        for (int i = 1; i < argc; i++) {
            sizes.push_back(static_cast<size_t>(std::stoul(argv[i])));
        }
    }

    std::string dataDir = "/tmp/minisql_benchmark";

    std::cout << "MiniSQL B-Tree vs Full Table Scan Benchmark\n";

    for (size_t size : sizes) {
        RunBenchmarkAtScale(dataDir, size);
    }

    std::filesystem::remove_all(dataDir);
    return 0;
}
