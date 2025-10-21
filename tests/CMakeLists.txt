add_executable(run_tests 
    basic_ir_test.cpp
    factorial_test.cpp
    use_def_test.cpp
    graph_analyzer_test.cpp
    helpers/factorial_graph.cpp
)

target_link_libraries(run_tests PRIVATE ir_core gtest_main)

include(GoogleTest)

gtest_discover_tests(run_tests)