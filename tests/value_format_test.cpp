#include <cstdint>
#include <cstdlib>

#include "console_calc/value_format.h"

namespace {

bool expect_integer_display_modes() {
    using console_calc::IntegerDisplayMode;
    using console_calc::ScalarValue;

    return console_calc::format_scalar(ScalarValue{std::int64_t{255}},
                                       IntegerDisplayMode::decimal) == "255" &&
           console_calc::format_scalar(ScalarValue{std::int64_t{255}},
                                       IntegerDisplayMode::hexadecimal) == "0xff" &&
           console_calc::format_scalar(ScalarValue{std::int64_t{255}},
                                       IntegerDisplayMode::binary) == "0b11111111" &&
           console_calc::format_scalar(ScalarValue{std::int64_t{-10}},
                                       IntegerDisplayMode::hexadecimal) == "-0xa" &&
           console_calc::format_scalar(ScalarValue{std::int64_t{-10}},
                                       IntegerDisplayMode::binary) == "-0b1010";
}

bool expect_float_display_modes() {
    using console_calc::IntegerDisplayMode;
    using console_calc::ScalarValue;

    return console_calc::format_scalar(ScalarValue{2.5}, IntegerDisplayMode::decimal) == "2.5" &&
           console_calc::format_scalar(ScalarValue{2.5}, IntegerDisplayMode::hexadecimal) ==
               "2.5" &&
           console_calc::format_scalar(ScalarValue{2.5}, IntegerDisplayMode::binary) == "2.5";
}

bool expect_list_display_modes() {
    using console_calc::IntegerDisplayMode;
    using console_calc::ListValue;
    using console_calc::ScalarValue;

    const ListValue values{ScalarValue{std::int64_t{15}}, ScalarValue{1.5},
                           ScalarValue{std::int64_t{-2}}};

    return console_calc::format_list(values, IntegerDisplayMode::decimal) == "{15, 1.5, -2}" &&
           console_calc::format_list(values, IntegerDisplayMode::hexadecimal) ==
               "{0xf, 1.5, -0x2}" &&
           console_calc::format_list(values, IntegerDisplayMode::binary) ==
               "{0b1111, 1.5, -0b10}";
}

}  // namespace

int main() {
    if (!expect_integer_display_modes()) {
        return EXIT_FAILURE;
    }

    if (!expect_float_display_modes()) {
        return EXIT_FAILURE;
    }

    if (!expect_list_display_modes()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
