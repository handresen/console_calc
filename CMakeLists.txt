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
option(CONSOLE_CALC_ENABLE_CLANG_TIDY "Run clang-tidy during builds when available" OFF)

if(CONSOLE_CALC_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    else()
        message(FATAL_ERROR "CONSOLE_CALC_ENABLE_CLANG_TIDY is ON, but clang-tidy was not found")
    endif()
endif()

add_library(console_calc_lib
    src/expression_parser.cpp
)

target_include_directories(console_calc_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(console_calc_lib PUBLIC cxx_std_20)

add_library(console_calc_app_lib
    apps/console_calc_app.cpp
    apps/console_mode.cpp
)

target_include_directories(console_calc_app_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/apps
)

target_link_libraries(console_calc_app_lib
    PRIVATE
        console_calc_lib
)

target_compile_features(console_calc_app_lib PUBLIC cxx_std_20)

add_executable(console_calc
    apps/console_calc_main.cpp
)

target_link_libraries(console_calc
    PRIVATE
        console_calc_app_lib
)

if(CONSOLE_CALC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
