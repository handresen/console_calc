add_executable(console_calc_expression_conformance_test
    expression_case_loader.cpp
    expression_conformance_test.cpp
)

target_link_libraries(console_calc_expression_conformance_test
    PRIVATE
        console_calc_lib
)

target_compile_definitions(console_calc_expression_conformance_test
    PRIVATE
        CONSOLE_CALC_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(
    NAME console_calc_expression_conformance_test
    COMMAND $<TARGET_FILE:console_calc_expression_conformance_test>
)

add_executable(console_calc_cli_test
    console_calc_cli_test.cpp
)

target_include_directories(console_calc_cli_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/apps
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(console_calc_cli_test
    PRIVATE
        console_calc_app_lib
)

add_test(NAME console_calc_cli_test COMMAND $<TARGET_FILE:console_calc_cli_test>)

add_executable(console_calc_history_test
    console_history_test.cpp
)

target_include_directories(console_calc_history_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/apps
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(console_calc_history_test
    PRIVATE
        console_calc_app_lib
)

add_test(NAME console_calc_history_test COMMAND $<TARGET_FILE:console_calc_history_test>)

add_executable(console_calc_session_test
    console_session_test.cpp
)

target_include_directories(console_calc_session_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/apps
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(console_calc_session_test
    PRIVATE
        console_calc_app_lib
)

add_test(NAME console_calc_session_test COMMAND $<TARGET_FILE:console_calc_session_test>)

add_executable(console_calc_console_command_test
    console_command_test.cpp
)

target_include_directories(console_calc_console_command_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/apps
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(console_calc_console_command_test
    PRIVATE
        console_calc_app_lib
)

add_test(NAME console_calc_console_command_test COMMAND $<TARGET_FILE:console_calc_console_command_test>)

add_executable(console_calc_value_format_test
    value_format_test.cpp
)

target_link_libraries(console_calc_value_format_test
    PRIVATE
        console_calc_lib
)

add_test(NAME console_calc_value_format_test COMMAND $<TARGET_FILE:console_calc_value_format_test>)

add_executable(console_calc_currency_support_test
    currency_support_test.cpp
)

target_include_directories(console_calc_currency_support_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/apps
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(console_calc_currency_support_test
    PRIVATE
        console_calc_app_lib
)

add_test(NAME console_calc_currency_support_test COMMAND $<TARGET_FILE:console_calc_currency_support_test>)
