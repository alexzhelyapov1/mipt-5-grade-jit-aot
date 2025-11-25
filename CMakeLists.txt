cmake_minimum_required(VERSION 3.20)
project(ir_core)

enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ir_core
    src/ir/types.h
    src/ir/instruction.h
    src/ir/instruction.cpp
    src/ir/basic_block.h
    src/ir/basic_block.cpp
    src/ir/graph.h
    src/ir/graph.cpp
    src/ir/ir_builder.h
    src/ir/ir_builder.cpp
    src/ir/ir.h
    src/ir/analysis/graph_analyzer.h
    src/ir/analysis/graph_analyzer.cpp
    src/ir/analysis/loop.h
    src/ir/analysis/loop.cpp
    src/ir/analysis/loop_analyzer.h
    src/ir/analysis/loop_analyzer.cpp
    src/ir/opt/peephole_optimizer.h
    src/ir/opt/peephole_optimizer.cpp
)

target_include_directories(ir_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(third-party/googletest)

enable_testing()

add_subdirectory(tests)

add_executable(dump_factorial_ir
    tests/dump_ir_main.cpp
    tests/helpers/factorial_graph.cpp
)
target_include_directories(dump_factorial_ir PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(dump_factorial_ir PRIVATE ir_core)
