cmake_minimum_required(VERSION 3.21)

project(YARVS
        LANGUAGES CXX)

if (${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
    message(FATAL_ERROR "In-source build is forbidden")
endif()

find_package(fmt REQUIRED)
find_package(CLI11 REQUIRED)
find_package(elfio REQUIRED)

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

add_executable(yarvs
    ${SOURCE_DIR}/main.cpp
    ${SOURCE_DIR}/elf_loader.cpp
)

set_target_properties(yarvs PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(yarvs
    PRIVATE ${INCLUDE_DIR}
)

target_link_libraries(yarvs
    PRIVATE CLI11::CLI11 elfio::elfio fmt::fmt
)
