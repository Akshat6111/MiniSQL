#pragma once

#include <fstream>
#include <string>

#include "storage/page.h"

namespace minisql {

class PageManager {
public:
    explicit PageManager(const std::string& filePath);

    uint32_t AllocatePage();
    void ReadPage(uint32_t pageId, Page& page) const;
    void WritePage(const Page& page);
    uint32_t GetPageCount() const;

private:
    std::string filePath_;
    mutable std::fstream file_;
    uint32_t pageCount_;

    void OpenOrCreateFile();
};

}  // namespace minisql
