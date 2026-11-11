#include "storage/page_manager.h"

#include "common/exceptions.h"

namespace minisql {

PageManager::PageManager(const std::string& filePath) : filePath_(filePath), pageCount_(0) {
    OpenOrCreateFile();

    file_.seekg(0, std::ios::end);
    std::streamoff fileSize = file_.tellg();
    pageCount_ = static_cast<uint32_t>(fileSize / static_cast<std::streamoff>(kPageSize));
}

void PageManager::OpenOrCreateFile() {
    file_.open(filePath_, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_.is_open()) {
        file_.clear();
        std::ofstream createStream(filePath_, std::ios::out | std::ios::binary);
        if (!createStream.is_open()) {
            throw MiniSqlException("Failed to create database file: " + filePath_);
        }
        createStream.close();
        file_.open(filePath_, std::ios::in | std::ios::out | std::ios::binary);
    }
    if (!file_.is_open()) {
        throw MiniSqlException("Failed to open database file: " + filePath_);
    }
}

uint32_t PageManager::AllocatePage() {
    uint32_t newPageId = pageCount_;
    Page blankPage(newPageId);

    file_.seekp(static_cast<std::streamoff>(newPageId) * static_cast<std::streamoff>(kPageSize));
    file_.write(reinterpret_cast<const char*>(blankPage.Data()), kPageSize);
    file_.flush();

    if (!file_) {
        throw MiniSqlException("Failed to allocate page in file: " + filePath_);
    }

    pageCount_++;
    return newPageId;
}

void PageManager::ReadPage(uint32_t pageId, Page& page) const {
    if (pageId >= pageCount_) {
        throw MiniSqlException("Attempted to read out-of-range page " + std::to_string(pageId) +
                                " (file has " + std::to_string(pageCount_) + " pages)");
    }

    file_.seekg(static_cast<std::streamoff>(pageId) * static_cast<std::streamoff>(kPageSize));
    file_.read(reinterpret_cast<char*>(page.Data()), kPageSize);

    if (!file_) {
        throw MiniSqlException("Failed to read page " + std::to_string(pageId) + " from file: " +
                                filePath_);
    }

    page.SetPageId(pageId);
}

void PageManager::WritePage(const Page& page) {
    file_.seekp(static_cast<std::streamoff>(page.GetPageId()) * static_cast<std::streamoff>(kPageSize));
    file_.write(reinterpret_cast<const char*>(page.Data()), kPageSize);
    file_.flush();

    if (!file_) {
        throw MiniSqlException("Failed to write page " + std::to_string(page.GetPageId()) +
                                " to file: " + filePath_);
    }
}

uint32_t PageManager::GetPageCount() const {
    return pageCount_;
}

}  // namespace minisql
