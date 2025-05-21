add_executable(unit_tests
    ./src/bit_manipulation.cpp
    ./src/executor.cpp
)

target_link_libraries(unit_tests
PRIVATE
    yarvs-lib
    fmt::fmt
    GTest::GTest
    GTest::gtest_main
    Threads::Threads
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
