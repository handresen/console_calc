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

add_executable(console_calc_app_test
    console_calc_app_test.cpp
)

target_link_libraries(console_calc_app_test
    PRIVATE
        console_calc_app_lib
)

add_test(NAME console_calc_app_test COMMAND $<TARGET_FILE:console_calc_app_test>)
