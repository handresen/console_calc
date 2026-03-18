#pragma once

#include <string>

#include "console_calc/value.h"

namespace console_calc {

[[nodiscard]] std::string format_scalar(const ScalarValue& value);
[[nodiscard]] std::string format_list(const ListValue& values);
[[nodiscard]] std::string format_value(const Value& value);

}  // namespace console_calc
