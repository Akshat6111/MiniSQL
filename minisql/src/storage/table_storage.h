#pragma once

#include <memory>
#include <string>
#include <vector>

#include "storage/page_manager.h"
#include "storage/row.h"
#include "storage/row_location.h"
#include "storage/schema.h"

namespace minisql {

constexpr uint32_t kCatalogPageId = 0;
constexpr uint32_t kFirstDataPageId = 1;
constexpr uint32_t kNoNextPage = 0;
constexpr size_t kDataPageHeaderSize = 8;

class TableStorage {
public:
    static std::unique_ptr<TableStorage> Create(const std::string& filePath, const Schema& schema);
    static std::unique_ptr<TableStorage> Open(const std::string& filePath);

    const Schema& GetSchema() const;
    RowLocation InsertRow(const Row& row);
    Row ReadRow(const RowLocation& location) const;
    std::vector<Row> ReadAllRows() const;

private:
    TableStorage(std::unique_ptr<PageManager> pageManager, Schema schema);

    std::unique_ptr<PageManager> pageManager_;
    Schema schema_;
    size_t rowSize_;
    size_t rowsPerPage_;
    uint32_t lastDataPageId_;

    uint32_t FindLastDataPageId() const;
};

}  // namespace minisql
