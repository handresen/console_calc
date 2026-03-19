#pragma once

#include <string>

#include "console_calc/value.h"

namespace console_calc {

enum class IntegerDisplayMode {
    decimal,
    hexadecimal,
    binary,
};

[[nodiscard]] std::string format_scalar(const ScalarValue& value);
[[nodiscard]] std::string format_scalar(const ScalarValue& value, IntegerDisplayMode mode);
[[nodiscard]] std::string format_position(const PositionValue& value);
[[nodiscard]] std::string format_list(const ListValue& values);
[[nodiscard]] std::string format_list(const ListValue& values, IntegerDisplayMode mode);
[[nodiscard]] std::string format_value(const Value& value);
[[nodiscard]] std::string format_value(const Value& value, IntegerDisplayMode mode);

}  // namespace console_calc
