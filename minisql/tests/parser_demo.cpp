#include <iostream>

#include "ast/ast.h"
#include "common/exceptions.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

using namespace minisql;

std::string ColumnTypeToString(AstColumnType type) {
    return type == AstColumnType::INT ? "INT" : "VARCHAR";
}

std::string BinaryOperatorToString(BinaryOperator op) {
    switch (op) {
        case BinaryOperator::EQUALS: return "=";
        case BinaryOperator::NOT_EQUALS: return "!=";
        case BinaryOperator::LESS_THAN: return "<";
        case BinaryOperator::GREATER_THAN: return ">";
        case BinaryOperator::LESS_EQUAL: return "<=";
        case BinaryOperator::GREATER_EQUAL: return ">=";
        case BinaryOperator::AND: return "AND";
        case BinaryOperator::OR: return "OR";
    }
    return "?";
}

void PrintExpression(const Expression* expr, int depth) {
    std::string indent(depth * 2, ' ');
    if (expr == nullptr) {
        return;
    }
    switch (expr->GetType()) {
        case ExpressionType::COLUMN_REFERENCE: {
            const auto* ref = static_cast<const ColumnReferenceExpression*>(expr);
            std::cout << indent << "Column(";
            if (!ref->tableName.empty()) {
                std::cout << ref->tableName << ".";
            }
            std::cout << ref->columnName << ")\n";
            break;
        }
        case ExpressionType::LITERAL: {
            const auto* lit = static_cast<const LiteralExpression*>(expr);
            std::cout << indent << "Literal(" << lit->value << ")\n";
            break;
        }
        case ExpressionType::BINARY: {
            const auto* bin = static_cast<const BinaryExpression*>(expr);
            std::cout << indent << "Binary(" << BinaryOperatorToString(bin->op) << ")\n";
            PrintExpression(bin->left.get(), depth + 1);
            PrintExpression(bin->right.get(), depth + 1);
            break;
        }
    }
}

void PrintStatement(const Statement* statement) {
    switch (statement->GetType()) {
        case StatementType::CREATE_TABLE: {
            const auto* create = static_cast<const CreateStatement*>(statement);
            std::cout << "CreateStatement: table=" << create->tableName << "\n";
            for (const ColumnDefinition& col : create->columns) {
                std::cout << "  column " << col.name << " " << ColumnTypeToString(col.type)
                          << (col.isPrimaryKey ? " PRIMARY KEY" : "") << "\n";
            }
            break;
        }
        case StatementType::INSERT: {
            const auto* insert = static_cast<const InsertStatement*>(statement);
            std::cout << "InsertStatement: table=" << insert->tableName << "\n";
            for (const auto& value : insert->values) {
                PrintExpression(value.get(), 1);
            }
            break;
        }
        case StatementType::SELECT: {
            const auto* select = static_cast<const SelectStatement*>(statement);
            std::cout << "SelectStatement: table=" << select->tableName << "\n";
            if (select->selectAll) {
                std::cout << "  columns: *\n";
            } else {
                std::cout << "  columns:";
                for (const std::string& col : select->columns) {
                    std::cout << " " << col;
                }
                std::cout << "\n";
            }
            if (select->join) {
                std::cout << "  join: " << select->join->tableName << " ON " << select->join->leftTable
                          << "." << select->join->leftColumn << " = " << select->join->rightTable << "."
                          << select->join->rightColumn << "\n";
            }
            if (select->whereClause) {
                std::cout << "  where:\n";
                PrintExpression(select->whereClause.get(), 2);
            }
            break;
        }
    }
    std::cout << "\n";
}

void ParseAndPrint(const std::string& sql) {
    std::cout << "SQL: " << sql << "\n";
    try {
        Lexer lexer(sql);
        Parser parser(lexer.Tokenize());
        std::unique_ptr<Statement> statement = parser.ParseStatement();
        PrintStatement(statement.get());
    } catch (const MiniSqlException& e) {
        std::cout << "  ERROR: " << e.what() << "\n\n";
    }
}

int main() {
    ParseAndPrint("CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR, age INT);");
    ParseAndPrint("INSERT INTO users VALUES (1, 'Alice', 30);");
    ParseAndPrint("SELECT * FROM users WHERE age >= 18;");
    ParseAndPrint("SELECT id, name FROM users WHERE age > 18 AND name = 'Alice';");
    ParseAndPrint(
        "SELECT users.name, orders.total FROM users JOIN orders ON users.id = orders.user_id "
        "WHERE users.age >= 21 OR users.name = 'Bob';");

    // Malformed statements to confirm the parser reports errors instead of crashing.
    ParseAndPrint("CREATE TABLE users (id INT PRIMARY, name VARCHAR);");
    ParseAndPrint("SELECT FROM users;");
    ParseAndPrint("SELECT * FROM;");

    return 0;
}
