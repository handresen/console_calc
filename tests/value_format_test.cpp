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

bool expect_position_list_display() {
    using console_calc::PositionListValue;

    const PositionListValue values{
        {.latitude_deg = 60.0, .longitude_deg = 10.0},
        {.latitude_deg = 61.0, .longitude_deg = 11.0},
    };

    return console_calc::format_position_list(values) ==
           "{pos(60, 10), pos(61, 11)}";
}

bool expect_multi_list_display() {
    using console_calc::IntegerDisplayMode;
    using console_calc::MultiListValue;
    using console_calc::ScalarValue;

    const MultiListValue values{
        {ScalarValue{std::int64_t{1}}, ScalarValue{2.5}},
        {ScalarValue{std::int64_t{3}}, ScalarValue{std::int64_t{4}}},
    };

    return console_calc::format_multi_list(values, IntegerDisplayMode::decimal) ==
               "{{1, 2.5}, {3, 4}}" &&
           console_calc::format_multi_list(values, IntegerDisplayMode::hexadecimal) ==
               "{{0x1, 2.5}, {0x3, 0x4}}";
}

bool expect_multi_position_list_display() {
    using console_calc::MultiPositionListValue;

    const MultiPositionListValue values{
        {
            {.latitude_deg = 60.0, .longitude_deg = 10.0},
            {.latitude_deg = 61.0, .longitude_deg = 11.0},
        },
        {
            {.latitude_deg = 62.0, .longitude_deg = 12.0},
        },
    };

    return console_calc::format_multi_position_list(values) ==
           "{{pos(60, 10), pos(61, 11)}, {pos(62, 12)}}";
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

    if (!expect_position_list_display()) {
        return EXIT_FAILURE;
    }

    if (!expect_multi_list_display()) {
        return EXIT_FAILURE;
    }

    if (!expect_multi_position_list_display()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
