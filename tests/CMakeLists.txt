add_executable(console_calc_smoke_test
    smoke_test.cpp
)

target_link_libraries(console_calc_smoke_test
    PRIVATE
        console_calc_lib
)

add_test(NAME console_calc_smoke_test COMMAND console_calc_smoke_test)
