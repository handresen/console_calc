add_executable(console_calc_smoke_test
    smoke_test.cpp
)

target_link_libraries(console_calc_smoke_test
    PRIVATE
        console_calc_lib
)

add_test(NAME console_calc_smoke_test COMMAND $<TARGET_FILE:console_calc_smoke_test>)

add_executable(console_calc_app_test
    console_calc_app_test.cpp
)

target_link_libraries(console_calc_app_test
    PRIVATE
        console_calc_app_lib
)

add_test(NAME console_calc_app_test COMMAND $<TARGET_FILE:console_calc_app_test>)
