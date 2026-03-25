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
using MultiListValue = std::vector<ListValue>;
using PositionListValue = std::vector<PositionValue>;
using MultiPositionListValue = std::vector<PositionListValue>;
using Value =
    std::variant<std::int64_t, double, ListValue, MultiListValue, PositionValue, PositionListValue,
                 MultiPositionListValue>;

}  // namespace console_calc
