#include <cassert>
#include <filesystem>
#include <iostream>

#include "common/exceptions.h"
#include "storage/storage_engine.h"

using namespace minisql;

void PrintRow(const Row& row, const Schema& schema) {
    const std::vector<Column>& columns = schema.GetColumns();
    for (size_t i = 0; i < columns.size(); i++) {
        std::cout << columns[i].name << "=" << ValueToString(row.GetValue(i));
        if (i + 1 < columns.size()) {
            std::cout << ", ";
        }
    }
    std::cout << "\n";
}

int main() {
    std::string dataDir = "/tmp/minisql_storage_demo";
    std::filesystem::remove_all(dataDir);

    Schema schema("users", {
                                Column{"id", ColumnType::INT, true},
                                Column{"name", ColumnType::VARCHAR, false},
                            });

    const int kRowCount = 150;

    {
        StorageEngine engine(dataDir);
        engine.CreateTable(schema);

        for (int i = 0; i < kRowCount; i++) {
            Row row({Value(static_cast<int32_t>(i)), Value(std::string("user_") + std::to_string(i))});
            engine.InsertRow("users", row);
        }

        std::vector<Row> rows = engine.ReadAllRows("users");
        std::cout << "Inserted " << kRowCount << " rows, read back " << rows.size() << " rows\n";
        assert(rows.size() == static_cast<size_t>(kRowCount));

        std::cout << "First row:  ";
        PrintRow(rows.front(), schema);
        std::cout << "Last row:   ";
        PrintRow(rows.back(), schema);

        for (int i = 0; i < kRowCount; i++) {
            assert(std::get<int32_t>(rows[i].GetValue(0)) == i);
            assert(std::get<std::string>(rows[i].GetValue(1)) == "user_" + std::to_string(i));
        }
        std::cout << "All " << kRowCount << " rows verified correct within this session.\n\n";
    }

    {
        std::cout << "Reopening StorageEngine on the same directory (fresh process would see this)...\n";
        StorageEngine engine(dataDir);
        std::vector<Row> rows = engine.ReadAllRows("users");
        assert(rows.size() == static_cast<size_t>(kRowCount));
        for (int i = 0; i < kRowCount; i++) {
            assert(std::get<int32_t>(rows[i].GetValue(0)) == i);
        }
        std::cout << "Persistence verified: " << rows.size() << " rows survived reopening the file.\n\n";

        try {
            engine.CreateTable(schema);
            std::cout << "ERROR: expected duplicate CREATE TABLE to throw\n";
        } catch (const MiniSqlException& e) {
            std::cout << "Correctly rejected duplicate CREATE TABLE: " << e.what() << "\n";
        }

        try {
            Row badRow({Value(std::string("not_an_int")), Value(std::string("x"))});
            engine.InsertRow("users", badRow);
            std::cout << "ERROR: expected type-mismatched INSERT to throw\n";
        } catch (const MiniSqlException& e) {
            std::cout << "Correctly rejected type-mismatched INSERT: " << e.what() << "\n";
        }

        try {
            engine.ReadAllRows("nonexistent_table");
            std::cout << "ERROR: expected read of unknown table to throw\n";
        } catch (const MiniSqlException& e) {
            std::cout << "Correctly rejected read of unknown table: " << e.what() << "\n";
        }
    }

    std::cout << "\nAll storage engine checks passed.\n";
    return 0;
}
