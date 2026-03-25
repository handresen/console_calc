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
option(CONSOLE_CALC_BUILD_TERMINAL_APP "Build the terminal console application" ON)
option(CONSOLE_CALC_ENABLE_NATIVE_CURRENCY_PROVIDER "Build the libcurl-backed native currency provider" ON)

if(CONSOLE_CALC_ENABLE_NATIVE_CURRENCY_PROVIDER)
    find_package(CURL REQUIRED)
endif()

find_package(GeographicLib CONFIG REQUIRED)

if(CONSOLE_CALC_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    else()
        message(FATAL_ERROR "CONSOLE_CALC_ENABLE_CLANG_TIDY is ON, but clang-tidy was not found")
    endif()
endif()

add_library(console_calc_lib
    src/builtin_function.cpp
    src/error_info.cpp
    src/expression_ast_parser.cpp
    src/expression_ast_special_forms.cpp
    src/expression_evaluator_builtins.cpp
    src/expression_evaluator.cpp
    src/expression_evaluator_special_forms.cpp
    src/expression_parser.cpp
    src/expression_tokenizer.cpp
    src/geodesy.cpp
    src/scalar_math.cpp
    src/scalar_value.cpp
    src/special_form.cpp
    src/value_format.cpp
    src/value_utils.cpp
)

target_include_directories(console_calc_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_compile_features(console_calc_lib PUBLIC cxx_std_20)

target_link_libraries(console_calc_lib
    PRIVATE
        ${GeographicLib_LIBRARIES}
)

add_library(console_calc_runtime_lib
    apps/console_assignment.cpp
    apps/console_command.cpp
    apps/console_command_executor.cpp
    apps/console_listing.cpp
    apps/console_session_engine.cpp
    apps/currency_definition_materializer.cpp
    apps/currency_rate_parser.cpp
    apps/currency_rate_provider.cpp
    apps/expression_environment.cpp
    apps/expression_expansion_support.cpp
)

target_include_directories(console_calc_runtime_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/apps
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(console_calc_runtime_lib
    PRIVATE
        console_calc_lib
)

if(CONSOLE_CALC_ENABLE_NATIVE_CURRENCY_PROVIDER)
    target_link_libraries(console_calc_runtime_lib
        PRIVATE
            CURL::libcurl
    )
endif()

target_compile_features(console_calc_runtime_lib PUBLIC cxx_std_20)

target_compile_definitions(console_calc_runtime_lib
    PUBLIC
        CONSOLE_CALC_ENABLE_NATIVE_CURRENCY_PROVIDER=$<BOOL:${CONSOLE_CALC_ENABLE_NATIVE_CURRENCY_PROVIDER}>
)

add_library(console_calc_host_lib
    bindings/facade/console_binding_facade.cpp
    bindings/c_api/console_binding_c_api.cpp
)

target_include_directories(console_calc_host_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/apps
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(console_calc_host_lib
    PRIVATE
        console_calc_runtime_lib
)

target_compile_features(console_calc_host_lib PUBLIC cxx_std_20)

if(EMSCRIPTEN)
    target_compile_options(console_calc_lib
        PRIVATE
            -fexceptions
    )

    target_compile_options(console_calc_runtime_lib
        PRIVATE
            -fexceptions
    )

    target_compile_options(console_calc_host_lib
        PRIVATE
            -fexceptions
    )

    add_executable(console_calc_wasm
        bindings/wasm/console_wasm_entry.cpp
    )

    target_link_libraries(console_calc_wasm
        PRIVATE
            console_calc_host_lib
    )

    target_compile_features(console_calc_wasm PUBLIC cxx_std_20)

    set_target_properties(console_calc_wasm PROPERTIES
        OUTPUT_NAME "console_calc"
        SUFFIX ".mjs"
    )

    target_link_options(console_calc_wasm
        PRIVATE
            "-fexceptions"
            "--no-entry"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sDISABLE_EXCEPTION_CATCHING=0"
            "-sENVIRONMENT=web"
            "-sEXPORT_ES6=1"
            "-sFILESYSTEM=0"
            "-sMODULARIZE=1"
            "-sEXPORTED_FUNCTIONS=['_console_calc_binding_session_create','_console_calc_binding_session_destroy','_console_calc_binding_session_initialize','_console_calc_binding_session_submit','_console_calc_binding_session_last_result_json','_free']"
            "-sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToNewUTF8']"
        )
endif()

if(CONSOLE_CALC_BUILD_TERMINAL_APP)
    add_library(console_calc_app_lib
        apps/console_calc_app.cpp
        apps/console_history.cpp
        apps/console_line_editor.cpp
        apps/console_mode.cpp
        apps/console_session.cpp
    )

    target_include_directories(console_calc_app_lib
        PUBLIC
            ${PROJECT_SOURCE_DIR}/apps
            ${PROJECT_SOURCE_DIR}/include
        PRIVATE
            ${PROJECT_SOURCE_DIR}/src
    )

    target_link_libraries(console_calc_app_lib
        PRIVATE
            console_calc_runtime_lib
    )

    target_compile_features(console_calc_app_lib PUBLIC cxx_std_20)

    add_executable(console_calc
        apps/console_calc_main.cpp
    )

    target_link_libraries(console_calc
        PRIVATE
            console_calc_app_lib
    )
endif()

if(CONSOLE_CALC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
