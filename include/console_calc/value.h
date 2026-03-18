#pragma once

#include <cstdint>
#include <variant>
#include <vector>

namespace console_calc {

using ScalarValue = std::variant<std::int64_t, double>;
using ListValue = std::vector<ScalarValue>;
using Value = std::variant<std::int64_t, double, ListValue>;

}  // namespace console_calc
