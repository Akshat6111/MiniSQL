#pragma once

#include <stdexcept>
#include <string>

namespace minisql {

class MiniSqlException : public std::runtime_error {
public:
    explicit MiniSqlException(const std::string& message) : std::runtime_error(message) {}
};

}  // namespace minisql
