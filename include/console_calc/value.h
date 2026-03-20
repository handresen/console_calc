#pragma once

#include <cstdint>
#include <variant>
#include <vector>

namespace console_calc {

struct PositionValue {
    double latitude_deg = 0.0;
    double longitude_deg = 0.0;
};

using ScalarValue = std::variant<std::int64_t, double>;
using ListValue = std::vector<ScalarValue>;
using PositionListValue = std::vector<PositionValue>;
using Value =
    std::variant<std::int64_t, double, ListValue, PositionValue, PositionListValue>;

}  // namespace console_calc
