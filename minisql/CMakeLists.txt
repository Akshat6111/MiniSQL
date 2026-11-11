cmake_minimum_required(VERSION 3.16)
project(MiniSQL CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(${CMAKE_SOURCE_DIR}/src)

# ==========================
# Lexer
# ==========================
add_library(minisql_lexer
    src/lexer/token.cpp
    src/lexer/lexer.cpp
)

# ==========================
# AST
# ==========================
add_library(minisql_ast
    src/ast/ast.cpp
)

# ==========================
# Parser
# ==========================
add_library(minisql_parser
    src/parser/parser.cpp
)
target_link_libraries(minisql_parser PRIVATE minisql_ast minisql_lexer)

# ==========================
# Lexer Demo
# ==========================
add_executable(lexer_demo
    tests/lexer_demo.cpp
)
target_link_libraries(lexer_demo PRIVATE minisql_lexer)

# ==========================
# Parser Demo
# ==========================
add_executable(parser_demo
    tests/parser_demo.cpp
)
target_link_libraries(parser_demo PRIVATE
    minisql_parser
    minisql_ast
    minisql_lexer
)

# ==========================
# Storage
# ==========================
add_library(minisql_storage
    src/storage/page.cpp
    src/storage/page_manager.cpp
    src/storage/schema.cpp
    src/storage/row.cpp
    src/storage/table_storage.cpp
    src/storage/storage_engine.cpp
)

# ==========================
# Storage Demo
# ==========================
add_executable(storage_demo
    tests/storage_demo.cpp
)
target_link_libraries(storage_demo PRIVATE minisql_storage)

# ==========================
# BTree
# ==========================
add_library(minisql_btree
    src/btree/btree_node.cpp
    src/btree/btree.cpp
)
target_link_libraries(minisql_btree PRIVATE minisql_storage)

# ==========================
# BTree Demo
# ==========================
add_executable(btree_demo
    tests/btree_demo.cpp
)
target_link_libraries(btree_demo PRIVATE
    minisql_btree
    minisql_storage
)

# ==========================
# Executor
# ==========================
add_library(minisql_executor
    src/executor/row_context.cpp
    src/executor/expression_evaluator.cpp
    src/executor/executor.cpp
)
target_link_libraries(minisql_executor PRIVATE
    minisql_ast
    minisql_storage
    minisql_btree
)

# ==========================
# Executor Demo
# ==========================
add_executable(executor_demo
    tests/executor_demo.cpp
)
target_link_libraries(executor_demo PRIVATE
    minisql_executor
    minisql_parser
    minisql_ast
    minisql_storage
    minisql_btree
    minisql_lexer
)

# ==========================
# Benchmark
# ==========================
add_library(minisql_benchmark
    src/benchmark/dataset_generator.cpp
    src/benchmark/benchmark_runner.cpp
)
target_link_libraries(minisql_benchmark PRIVATE
    minisql_storage
    minisql_btree
)
target_compile_options(minisql_benchmark PRIVATE -O2)

add_executable(benchmark
    src/benchmark/benchmark_main.cpp
)
target_link_libraries(benchmark PRIVATE
    minisql_benchmark
    minisql_storage
    minisql_btree
)
target_compile_options(benchmark PRIVATE -O2)

add_executable(query_runner
    tests/query_runner.cpp
)

target_link_libraries(query_runner PRIVATE
    minisql_executor
    minisql_parser
    minisql_ast
    minisql_storage
    minisql_btree
    minisql_lexer
)