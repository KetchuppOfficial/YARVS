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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CODEGEN_DIR ${PROJECT_BINARY_DIR}/code-gen)

# YARVS library

add_library(yarvs-lib STATIC
    ./src/hart.cpp
    ./src/executor.cpp
    ./src/elf_loader.cpp
    ${CODEGEN_DIR}/src/decoder.cpp
    ${CODEGEN_DIR}/src/instruction.cpp
)
target_include_directories(yarvs-lib
PUBLIC
    ./include
    ${CODEGEN_DIR}/include
)
target_link_libraries(yarvs-lib
PRIVATE
    elfio::elfio
    fmt::fmt
)
target_compile_features(yarvs-lib PUBLIC cxx_std_23)
set_target_properties(yarvs-lib PROPERTIES OUTPUT_NAME yarvs)
add_dependencies(yarvs-lib code_generator)

# YARVS simulator

add_executable(yarvs-exe ./src/main.cpp)

set_target_properties(yarvs-exe PROPERTIES OUTPUT_NAME yarvs)

target_link_libraries(yarvs-exe
    PRIVATE yarvs-lib CLI11::CLI11 fmt::fmt
)

add_subdirectory(code-gen)
add_subdirectory(test)
