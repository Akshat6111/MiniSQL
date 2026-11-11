#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "ast/ast.h"
#include "btree/btree.h"
#include "executor/execution_result.h"
#include "storage/storage_engine.h"

namespace minisql {

class Executor {
public:
    Executor(StorageEngine& storageEngine, std::string dataDirectory);

    ResultSet Execute(const Statement* statement);

private:
    StorageEngine& storageEngine_;
    std::string dataDirectory_;
    std::map<std::string, std::unique_ptr<BTree>> primaryKeyIndexes_;
    std::map<std::string, std::string> primaryKeyColumnNames_;

    ResultSet ExecuteCreate(const CreateStatement* create);
    ResultSet ExecuteInsert(const InsertStatement* insert);
    ResultSet ExecuteSelect(const SelectStatement* select);
    ResultSet ExecuteSingleTableSelect(const SelectStatement* select);
    ResultSet ExecuteJoinSelect(const SelectStatement* select);

    BTree* GetOrLoadIndex(const std::string& tableName);
    std::string IndexFilePath(const std::string& tableName, const std::string& columnName) const;

    static Value LiteralToValue(const LiteralExpression* literal, const Column& column);
    std::optional<std::pair<int32_t, int32_t>> TryExtractIndexRange(const Expression* expr,
                                                                      const std::string& tableName,
                                                                      const std::string& pkColumn) const;
};

}  // namespace minisql
