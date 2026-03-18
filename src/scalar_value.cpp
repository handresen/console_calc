#include "console_calc/scalar_value.h"

#include <variant>

namespace console_calc {

bool is_integer_scalar(const ScalarValue& value) {
    return std::holds_alternative<std::int64_t>(value);
}

double scalar_to_double(const ScalarValue& value) {
    if (const auto* integer = std::get_if<std::int64_t>(&value)) {
        return static_cast<double>(*integer);
    }

    return std::get<double>(value);
}

std::optional<std::int64_t> try_get_int64(const ScalarValue& value) {
    if (const auto* integer = std::get_if<std::int64_t>(&value)) {
        return *integer;
    }

    return std::nullopt;
}

Value to_value(const ScalarValue& value) {
    if (const auto* integer = std::get_if<std::int64_t>(&value)) {
        return *integer;
    }

    return std::get<double>(value);
}

}  // namespace console_calc
