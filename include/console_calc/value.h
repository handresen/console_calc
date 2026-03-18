#pragma once

#include <variant>
#include <vector>

namespace console_calc {

using ListValue = std::vector<double>;
using Value = std::variant<double, ListValue>;

}  // namespace console_calc
