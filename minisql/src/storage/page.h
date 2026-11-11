#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace minisql {

constexpr size_t kPageSize = 4096;

class Page {
public:
    Page();
    explicit Page(uint32_t pageId);

    uint32_t GetPageId() const;
    void SetPageId(uint32_t pageId);

    uint8_t* Data();
    const uint8_t* Data() const;

    void WriteUint32(size_t offset, uint32_t value);
    uint32_t ReadUint32(size_t offset) const;

    void WriteInt32(size_t offset, int32_t value);
    int32_t ReadInt32(size_t offset) const;

    void WriteBytes(size_t offset, const uint8_t* source, size_t length);
    void ReadBytes(size_t offset, uint8_t* destination, size_t length) const;

    void WriteString(size_t offset, const std::string& value, size_t fixedLength);
    std::string ReadString(size_t offset, size_t fixedLength) const;

    void Clear();

private:
    uint32_t pageId_;
    std::array<uint8_t, kPageSize> buffer_;

    void CheckBounds(size_t offset, size_t length) const;
};

}  // namespace minisql
