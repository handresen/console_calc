#pragma once

#include <cstdint>
#include <optional>

#include "console_calc/value.h"

namespace console_calc {

[[nodiscard]] bool is_integer_scalar(const ScalarValue& value);
[[nodiscard]] double scalar_to_double(const ScalarValue& value);
[[nodiscard]] std::optional<std::int64_t> try_get_int64(const ScalarValue& value);
[[nodiscard]] Value to_value(const ScalarValue& value);

}  // namespace console_calc
