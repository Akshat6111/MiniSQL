#include "storage/table_storage.h"

#include <utility>

#include "common/exceptions.h"

namespace minisql {

TableStorage::TableStorage(std::unique_ptr<PageManager> pageManager, Schema schema)
    : pageManager_(std::move(pageManager)),
      schema_(std::move(schema)),
      rowSize_(schema_.GetRowSize()),
      rowsPerPage_((kPageSize - kDataPageHeaderSize) / rowSize_),
      lastDataPageId_(kFirstDataPageId) {
    if (rowsPerPage_ == 0) {
        throw MiniSqlException("Row size " + std::to_string(rowSize_) + " does not fit in a " +
                                std::to_string(kPageSize) + "-byte page");
    }
    lastDataPageId_ = FindLastDataPageId();
}

std::unique_ptr<TableStorage> TableStorage::Create(const std::string& filePath, const Schema& schema) {
    auto pageManager = std::make_unique<PageManager>(filePath);
    if (pageManager->GetPageCount() != 0) {
        throw MiniSqlException("Cannot create table: file already contains data: " + filePath);
    }

    uint32_t catalogPageId = pageManager->AllocatePage();
    Page catalogPage(catalogPageId);
    schema.WriteToPage(catalogPage);
    pageManager->WritePage(catalogPage);

    uint32_t firstDataPageId = pageManager->AllocatePage();
    Page firstDataPage(firstDataPageId);
    firstDataPage.WriteUint32(0, 0);              // rowCount
    firstDataPage.WriteUint32(4, kNoNextPage);     // nextPageId
    pageManager->WritePage(firstDataPage);

    return std::unique_ptr<TableStorage>(new TableStorage(std::move(pageManager), schema));
}

std::unique_ptr<TableStorage> TableStorage::Open(const std::string& filePath) {
    auto pageManager = std::make_unique<PageManager>(filePath);
    if (pageManager->GetPageCount() < 2) {
        throw MiniSqlException("Cannot open table: file is missing catalog or data pages: " + filePath);
    }

    Page catalogPage(kCatalogPageId);
    pageManager->ReadPage(kCatalogPageId, catalogPage);
    Schema schema = Schema::ReadFromPage(catalogPage);

    return std::unique_ptr<TableStorage>(new TableStorage(std::move(pageManager), std::move(schema)));
}

const Schema& TableStorage::GetSchema() const {
    return schema_;
}

uint32_t TableStorage::FindLastDataPageId() const {
    uint32_t pageId = kFirstDataPageId;
    Page page(pageId);
    while (true) {
        pageManager_->ReadPage(pageId, page);
        uint32_t nextPageId = page.ReadUint32(4);
        if (nextPageId == kNoNextPage) {
            return pageId;
        }
        pageId = nextPageId;
    }
}

RowLocation TableStorage::InsertRow(const Row& row) {
    Page page(lastDataPageId_);
    pageManager_->ReadPage(lastDataPageId_, page);
    uint32_t rowCount = page.ReadUint32(0);

    if ((rowCount + 1) * rowSize_ + kDataPageHeaderSize > kPageSize) {
        uint32_t newPageId = pageManager_->AllocatePage();
        page.WriteUint32(4, newPageId);  // link previous last page to the new one
        pageManager_->WritePage(page);

        page = Page(newPageId);
        page.WriteUint32(0, 0);
        page.WriteUint32(4, kNoNextPage);
        lastDataPageId_ = newPageId;
        rowCount = 0;
    }

    size_t offset = kDataPageHeaderSize + static_cast<size_t>(rowCount) * rowSize_;
    row.SerializeTo(page, offset, schema_);
    page.WriteUint32(0, rowCount + 1);
    pageManager_->WritePage(page);

    return RowLocation{lastDataPageId_, rowCount};
}

Row TableStorage::ReadRow(const RowLocation& location) const {
    Page page(location.pageId);
    pageManager_->ReadPage(location.pageId, page);

    uint32_t rowCount = page.ReadUint32(0);
    if (location.slotIndex >= rowCount) {
        throw MiniSqlException("Row location slot " + std::to_string(location.slotIndex) +
                                " is out of range on page " + std::to_string(location.pageId));
    }

    size_t offset = kDataPageHeaderSize + static_cast<size_t>(location.slotIndex) * rowSize_;
    return Row::DeserializeFrom(page, offset, schema_);
}

std::vector<Row> TableStorage::ReadAllRows() const {
    std::vector<Row> rows;
    uint32_t pageId = kFirstDataPageId;
    Page page(pageId);

    while (true) {
        pageManager_->ReadPage(pageId, page);
        uint32_t rowCount = page.ReadUint32(0);

        for (uint32_t i = 0; i < rowCount; i++) {
            size_t offset = kDataPageHeaderSize + static_cast<size_t>(i) * rowSize_;
            rows.push_back(Row::DeserializeFrom(page, offset, schema_));
        }

        uint32_t nextPageId = page.ReadUint32(4);
        if (nextPageId == kNoNextPage) {
            break;
        }
        pageId = nextPageId;
    }

    return rows;
}

}  // namespace minisql
