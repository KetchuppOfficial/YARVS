cmake_minimum_required(VERSION 3.21)

project(YARVS
        LANGUAGES CXX)

if (${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
    message(FATAL_ERROR "In-source build is forbidden")
endif()

find_package(fmt REQUIRED)
find_package(CLI11 REQUIRED)
find_package(elfio REQUIRED)
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
find_package(Python REQUIRED COMPONENTS Interpreter)

enable_testing()

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(CODEGEN_DIR ${PROJECT_SOURCE_DIR}/code-gen)

set(BUILD_SOURCE_DIR ${PROJECT_BINARY_DIR}/src)
set(BUILD_INCLUDE_DIR ${PROJECT_BINARY_DIR}/include)

# YARVS library

add_library(yarvs-lib STATIC
    ${SOURCE_DIR}/hart.cpp
    ${SOURCE_DIR}/executor.cpp
    ${BUILD_SOURCE_DIR}/decoder.cpp
)

add_dependencies(yarvs-lib code_generator)

set_target_properties(yarvs-lib PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(yarvs-lib
    PUBLIC ${INCLUDE_DIR} ${BUILD_INCLUDE_DIR}
)

target_link_libraries(yarvs-lib
    PRIVATE elfio::elfio
)

# YARVS simulator

add_executable(yarvs
    ${SOURCE_DIR}/main.cpp
)

set_target_properties(yarvs PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(yarvs
    PRIVATE yarvs-lib CLI11::CLI11 fmt::fmt
)

add_subdirectory(code-gen)
add_subdirectory(test)
