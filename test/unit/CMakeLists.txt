aux_source_directory(./src SRC_LIST)

add_executable(unit_tests ${SRC_LIST})

set_target_properties(unit_tests PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(unit_tests
    PRIVATE yarvs-lib GTest::GTest GTest::gtest_main Threads::Threads
)

target_include_directories(unit_tests
    PRIVATE ${INCLUDE_DIR} ${BUILD_INCLUDE_DIR}
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
