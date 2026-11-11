#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include "common/exceptions.h"
#include "executor/executor.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "storage/storage_engine.h"

using namespace minisql;

int main() {
    std::string dataDir = "./database";
    std::filesystem::create_directories(dataDir);

    StorageEngine storage(dataDir);
    Executor executor(storage, dataDir);

    std::string sql;

    while (std::getline(std::cin, sql)) {
        if (sql == "EXIT")
            break;

        try {
            Lexer lexer(sql);
            Parser parser(lexer.Tokenize());

            auto stmt = parser.ParseStatement();
            ResultSet result = executor.Execute(stmt.get());

            std::cout << "OK\n";

            for (const auto& col : result.columnNames)
                std::cout << col << ",";

            std::cout << "\n";

            for (const auto& row : result.rows) {
                for (const auto& cell : row)
                    std::cout << ValueToString(cell) << ",";

                std::cout << "\n";
            }

            std::cout << "__END__\n";
            std::cout.flush();
        }
        catch (const std::exception& e) {
            std::cout << "ERROR:" << e.what() << "\n";
            std::cout << "__END__\n";
            std::cout.flush();
        }
    }
}