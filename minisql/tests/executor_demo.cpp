#include <cassert>
#include <filesystem>
#include <iomanip>
#include <iostream>

#include "common/exceptions.h"
#include "executor/executor.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "storage/storage_engine.h"

using namespace minisql;

void PrintResultSet(const ResultSet& result) {
    for (size_t i = 0; i < result.columnNames.size(); i++) {
        std::cout << std::left << std::setw(14) << result.columnNames[i];
    }
    std::cout << "\n";

    for (const std::vector<Value>& row : result.rows) {
        for (const Value& cell : row) {
            std::cout << std::left << std::setw(14) << ValueToString(cell);
        }
        std::cout << "\n";
    }

    std::cout << "(" << result.rows.size() << " row" << (result.rows.size() == 1 ? "" : "s") << ", "
              << (result.usedIndex ? "used B-Tree index, " : "full table scan, ") << result.rowsExamined
              << " row" << (result.rowsExamined == 1 ? "" : "s") << " examined)\n";
}

ResultSet RunSql(Executor& executor, const std::string& sql) {
    std::cout << "SQL: " << sql << "\n";
    Lexer lexer(sql);
    Parser parser(lexer.Tokenize());
    std::unique_ptr<Statement> statement = parser.ParseStatement();
    ResultSet result = executor.Execute(statement.get());
    PrintResultSet(result);
    std::cout << "\n";
    return result;
}

int main() {
    std::string dataDir = "/tmp/minisql_executor_demo";
    std::filesystem::remove_all(dataDir);

    StorageEngine storage(dataDir);
    Executor executor(storage, dataDir);

    RunSql(executor, "CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR, age INT);");
    RunSql(executor, "CREATE TABLE orders (order_id INT PRIMARY KEY, user_id INT, total INT);");

    RunSql(executor, "INSERT INTO users VALUES (1, 'Alice', 30);");
    RunSql(executor, "INSERT INTO users VALUES (2, 'Bob', 25);");
    RunSql(executor, "INSERT INTO users VALUES (3, 'Carol', 40);");
    RunSql(executor, "INSERT INTO users VALUES (4, 'Dave', 22);");
    RunSql(executor, "INSERT INTO users VALUES (5, 'Eve', 35);");

    RunSql(executor, "INSERT INTO orders VALUES (100, 1, 250);");
    RunSql(executor, "INSERT INTO orders VALUES (101, 1, 75);");
    RunSql(executor, "INSERT INTO orders VALUES (102, 2, 400);");
    RunSql(executor, "INSERT INTO orders VALUES (103, 5, 60);");

    std::cout << "=== Point lookup on primary key (should use index) ===\n";
    ResultSet r1 = RunSql(executor, "SELECT * FROM users WHERE id = 3;");
    assert(r1.usedIndex);
    assert(r1.rows.size() == 1);
    assert(std::get<std::string>(r1.rows[0][1]) == "Carol");

    std::cout << "=== Range lookup on primary key (should use index) ===\n";
    ResultSet r2 = RunSql(executor, "SELECT id, name FROM users WHERE id >= 2 AND id <= 4;");
    assert(r2.usedIndex);
    assert(r2.rows.size() == 3);

    std::cout << "=== Filter on a non-indexed column (should full scan) ===\n";
    ResultSet r3 = RunSql(executor, "SELECT name, age FROM users WHERE age >= 25 AND age <= 35;");
    assert(!r3.usedIndex);
    assert(r3.rows.size() == 3);

    std::cout << "=== OR clause disables index planning, falls back to scan ===\n";
    ResultSet r4 = RunSql(executor, "SELECT id, name FROM users WHERE id = 1 OR id = 5;");
    assert(!r4.usedIndex);
    assert(r4.rows.size() == 2);

    std::cout << "=== JOIN across two tables ===\n";
    ResultSet r5 = RunSql(executor,
                           "SELECT users.name, orders.total FROM users JOIN orders ON users.id = "
                           "orders.user_id WHERE orders.total > 100;");
    assert(r5.rows.size() == 2);

    std::cout << "=== Error handling ===\n";
    try {
        RunSql(executor, "SELECT * FROM users WHERE nonexistent_column = 1;");
    } catch (const MiniSqlException& e) {
        std::cout << "Correctly rejected unknown column: " << e.what() << "\n\n";
    }

    try {
        RunSql(executor, "INSERT INTO users VALUES (1, 'Duplicate', 99);");
    } catch (const MiniSqlException& e) {
        std::cout << "Correctly rejected duplicate primary key: " << e.what() << "\n\n";
    }

    try {
        RunSql(executor, "SELECT * FORM users;");
    } catch (const MiniSqlException& e) {
        std::cout << "Correctly rejected malformed SQL: " << e.what() << "\n\n";
    }

    std::cout << "All executor checks passed.\n";
    return 0;
}
