#include "benchmark/dataset_generator.h"

#include <filesystem>
#include <utility>

#include "benchmark/timer.h"
#include "common/exceptions.h"

namespace minisql {

DatasetGenerator::DatasetGenerator(StorageEngine& storage, std::string dataDirectory)
    : storage_(storage), dataDirectory_(std::move(dataDirectory)) {}

BTree& DatasetGenerator::GetIndex() {
    if (!index_) {
        throw MiniSqlException("No dataset has been generated yet");
    }
    return *index_;
}

GenerationStats DatasetGenerator::Generate(const std::string& tableName, size_t rowCount) {
    std::string dbPath = dataDirectory_ + "/" + tableName + ".db";
    std::string indexPath = dataDirectory_ + "/" + tableName + "_id.idx";
    std::filesystem::remove(dbPath);
    std::filesystem::remove(indexPath);

    Schema schema(tableName, {
                                  Column{"id", ColumnType::INT, true},
                                  Column{"name", ColumnType::VARCHAR, false},
                                  Column{"age", ColumnType::INT, false},
                              });
    storage_.CreateTable(schema);
    index_ = BTree::Create(indexPath);

    Stopwatch stopwatch;
    for (size_t i = 0; i < rowCount; i++) {
        int32_t id = static_cast<int32_t>(i);
        Row row({Value(id), Value(std::string("user_") + std::to_string(id)),
                 Value(static_cast<int32_t>(20 + (id % 50)))});
        RowLocation location = storage_.InsertRow(tableName, row);
        index_->Insert(id, location);
    }
    double elapsedMs = stopwatch.ElapsedMilliseconds();

    double rowsPerSecond = elapsedMs > 0.0 ? static_cast<double>(rowCount) / (elapsedMs / 1000.0) : 0.0;
    return GenerationStats{rowCount, elapsedMs, rowsPerSecond};
}

}  // namespace minisql
