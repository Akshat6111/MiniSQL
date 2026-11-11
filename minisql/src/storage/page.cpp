#include "storage/page.h"

#include <cstring>

#include "common/exceptions.h"

namespace minisql {

Page::Page() : pageId_(0) {
    Clear();
}

Page::Page(uint32_t pageId) : pageId_(pageId) {
    Clear();
}

uint32_t Page::GetPageId() const {
    return pageId_;
}

void Page::SetPageId(uint32_t pageId) {
    pageId_ = pageId;
}

uint8_t* Page::Data() {
    return buffer_.data();
}

const uint8_t* Page::Data() const {
    return buffer_.data();
}

void Page::CheckBounds(size_t offset, size_t length) const {
    if (offset + length > kPageSize) {
        throw MiniSqlException("Page access out of bounds: offset=" + std::to_string(offset) +
                                " length=" + std::to_string(length));
    }
}

void Page::WriteUint32(size_t offset, uint32_t value) {
    CheckBounds(offset, sizeof(uint32_t));
    std::memcpy(buffer_.data() + offset, &value, sizeof(uint32_t));
}

uint32_t Page::ReadUint32(size_t offset) const {
    CheckBounds(offset, sizeof(uint32_t));
    uint32_t value = 0;
    std::memcpy(&value, buffer_.data() + offset, sizeof(uint32_t));
    return value;
}

void Page::WriteInt32(size_t offset, int32_t value) {
    CheckBounds(offset, sizeof(int32_t));
    std::memcpy(buffer_.data() + offset, &value, sizeof(int32_t));
}

int32_t Page::ReadInt32(size_t offset) const {
    CheckBounds(offset, sizeof(int32_t));
    int32_t value = 0;
    std::memcpy(&value, buffer_.data() + offset, sizeof(int32_t));
    return value;
}

void Page::WriteBytes(size_t offset, const uint8_t* source, size_t length) {
    CheckBounds(offset, length);
    std::memcpy(buffer_.data() + offset, source, length);
}

void Page::ReadBytes(size_t offset, uint8_t* destination, size_t length) const {
    CheckBounds(offset, length);
    std::memcpy(destination, buffer_.data() + offset, length);
}

void Page::WriteString(size_t offset, const std::string& value, size_t fixedLength) {
    CheckBounds(offset, fixedLength);
    if (value.size() > fixedLength) {
        throw MiniSqlException("String value '" + value + "' exceeds fixed column length of " +
                                std::to_string(fixedLength) + " bytes");
    }
    std::memset(buffer_.data() + offset, 0, fixedLength);
    std::memcpy(buffer_.data() + offset, value.data(), value.size());
}

std::string Page::ReadString(size_t offset, size_t fixedLength) const {
    CheckBounds(offset, fixedLength);
    const char* start = reinterpret_cast<const char*>(buffer_.data() + offset);
    size_t actualLength = strnlen(start, fixedLength);
    return std::string(start, actualLength);
}

void Page::Clear() {
    buffer_.fill(0);
}

}  // namespace minisql
