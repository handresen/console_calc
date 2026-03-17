cmake_minimum_required(VERSION 3.24)

project(
    console_calc
    VERSION 0.1.0
    DESCRIPTION "Command-line calculator with expression parsing"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CONSOLE_CALC_BUILD_TESTS "Build tests" ON)

add_library(console_calc_lib
    src/expression_parser.cpp
)

target_include_directories(console_calc_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(console_calc_lib PUBLIC cxx_std_20)

add_executable(console_calc
    apps/console_calc_main.cpp
)

target_link_libraries(console_calc
    PRIVATE
        console_calc_lib
)

if(CONSOLE_CALC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
