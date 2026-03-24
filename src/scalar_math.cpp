#include "scalar_math.h"

#include "console_calc/expression_error.h"
#include "console_calc/scalar_value.h"

#include <cmath>
#include <cstdint>
#include <limits>

namespace console_calc {

namespace {

double require_finite_result(double value) {
    if (!std::isfinite(value)) {
        throw EvaluationError("expression produced a non-finite result");
    }

    return value;
}

std::int64_t require_integer_operand(const ScalarValue& value) {
    if (const auto integer = try_get_int64(value); integer.has_value()) {
        return *integer;
    }

    const double numeric_value = scalar_to_double(value);
    if (!std::isfinite(numeric_value)) {
        throw EvaluationError("bitwise operators require 64-bit integer operands");
    }

    double integral_part = 0.0;
    if (std::modf(numeric_value, &integral_part) != 0.0) {
        throw EvaluationError("bitwise operators require 64-bit integer operands");
    }

    const long double integral_value = static_cast<long double>(integral_part);
    const long double min_value = static_cast<long double>(std::numeric_limits<std::int64_t>::min());
    const long double max_value = static_cast<long double>(std::numeric_limits<std::int64_t>::max());
    if (integral_value < min_value || integral_value > max_value) {
        throw EvaluationError("bitwise operators require 64-bit integer operands");
    }

    return static_cast<std::int64_t>(integral_part);
}

}  // namespace

std::optional<std::int64_t> checked_add_int64(std::int64_t lhs, std::int64_t rhs) {
    if ((rhs > 0 && lhs > std::numeric_limits<std::int64_t>::max() - rhs) ||
        (rhs < 0 && lhs < std::numeric_limits<std::int64_t>::min() - rhs)) {
        return std::nullopt;
    }

    return lhs + rhs;
}

std::optional<std::int64_t> checked_subtract_int64(std::int64_t lhs, std::int64_t rhs) {
    if ((rhs < 0 && lhs > std::numeric_limits<std::int64_t>::max() + rhs) ||
        (rhs > 0 && lhs < std::numeric_limits<std::int64_t>::min() + rhs)) {
        return std::nullopt;
    }

    return lhs - rhs;
}

std::optional<std::int64_t> checked_multiply_int64(std::int64_t lhs, std::int64_t rhs) {
    if (lhs == 0 || rhs == 0) {
        return std::int64_t{0};
    }

    if (lhs == -1) {
        if (rhs == std::numeric_limits<std::int64_t>::min()) {
            return std::nullopt;
        }
        return -rhs;
    }
    if (rhs == -1) {
        if (lhs == std::numeric_limits<std::int64_t>::min()) {
            return std::nullopt;
        }
        return -lhs;
    }

    if (lhs > 0) {
        if ((rhs > 0 && lhs > std::numeric_limits<std::int64_t>::max() / rhs) ||
            (rhs < 0 && rhs < std::numeric_limits<std::int64_t>::min() / lhs)) {
            return std::nullopt;
        }
    } else {
        if ((rhs > 0 && lhs < std::numeric_limits<std::int64_t>::min() / rhs) ||
            (rhs < 0 && lhs != 0 && rhs < std::numeric_limits<std::int64_t>::max() / lhs)) {
            return std::nullopt;
        }
    }

    return lhs * rhs;
}

std::optional<std::int64_t> checked_power_int64(std::int64_t base, std::int64_t exponent) {
    if (exponent < 0) {
        return std::nullopt;
    }

    std::int64_t total = 1;
    std::int64_t factor = base;
    std::int64_t remaining = exponent;
    while (remaining > 0) {
        if ((remaining & 1) != 0) {
            const auto multiplied = checked_multiply_int64(total, factor);
            if (!multiplied.has_value()) {
                return std::nullopt;
            }
            total = *multiplied;
        }
        remaining >>= 1;
        if (remaining > 0) {
            const auto squared = checked_multiply_int64(factor, factor);
            if (!squared.has_value()) {
                return std::nullopt;
            }
            factor = *squared;
        }
    }

    return total;
}

ScalarValue add_scalars(const ScalarValue& lhs, const ScalarValue& rhs) {
    const auto lhs_integer = try_get_int64(lhs);
    const auto rhs_integer = try_get_int64(rhs);
    if (lhs_integer.has_value() && rhs_integer.has_value()) {
        if (const auto result = checked_add_int64(*lhs_integer, *rhs_integer); result.has_value()) {
            return *result;
        }
    }

    return require_finite_result(scalar_to_double(lhs) + scalar_to_double(rhs));
}

ScalarValue subtract_scalars(const ScalarValue& lhs, const ScalarValue& rhs) {
    const auto lhs_integer = try_get_int64(lhs);
    const auto rhs_integer = try_get_int64(rhs);
    if (lhs_integer.has_value() && rhs_integer.has_value()) {
        if (const auto result =
                checked_subtract_int64(*lhs_integer, *rhs_integer); result.has_value()) {
            return *result;
        }
    }

    return require_finite_result(scalar_to_double(lhs) - scalar_to_double(rhs));
}

ScalarValue multiply_scalars(const ScalarValue& lhs, const ScalarValue& rhs) {
    const auto lhs_integer = try_get_int64(lhs);
    const auto rhs_integer = try_get_int64(rhs);
    if (lhs_integer.has_value() && rhs_integer.has_value()) {
        if (const auto result =
                checked_multiply_int64(*lhs_integer, *rhs_integer); result.has_value()) {
            return *result;
        }
    }

    return require_finite_result(scalar_to_double(lhs) * scalar_to_double(rhs));
}

ScalarValue divide_scalars(const ScalarValue& lhs, const ScalarValue& rhs) {
    const double rhs_numeric = scalar_to_double(rhs);
    if (rhs_numeric == 0.0) {
        throw EvaluationError("division by zero");
    }

    return require_finite_result(scalar_to_double(lhs) / rhs_numeric);
}

ScalarValue modulo_scalars(const ScalarValue& lhs, const ScalarValue& rhs) {
    const auto lhs_integer = try_get_int64(lhs);
    const auto rhs_integer = try_get_int64(rhs);
    if (lhs_integer.has_value() && rhs_integer.has_value()) {
        if (*rhs_integer == 0) {
            throw EvaluationError("modulo by zero");
        }
        return *lhs_integer % *rhs_integer;
    }

    const double rhs_numeric = scalar_to_double(rhs);
    if (rhs_numeric == 0.0) {
        throw EvaluationError("modulo by zero");
    }

    return require_finite_result(std::fmod(scalar_to_double(lhs), rhs_numeric));
}

ScalarValue power_scalars(const ScalarValue& lhs, const ScalarValue& rhs) {
    const auto lhs_integer = try_get_int64(lhs);
    const auto rhs_integer = try_get_int64(rhs);
    if (lhs_integer.has_value() && rhs_integer.has_value()) {
        if (const auto result = checked_power_int64(*lhs_integer, *rhs_integer); result.has_value()) {
            return *result;
        }
    }

    return require_finite_result(std::pow(scalar_to_double(lhs), scalar_to_double(rhs)));
}

ScalarValue compare_equal_scalars(const ScalarValue& lhs, const ScalarValue& rhs) {
    return static_cast<std::int64_t>(scalar_to_double(lhs) == scalar_to_double(rhs));
}

ScalarValue compare_less_scalars(const ScalarValue& lhs, const ScalarValue& rhs) {
    return static_cast<std::int64_t>(scalar_to_double(lhs) < scalar_to_double(rhs));
}

ScalarValue compare_less_equal_scalars(const ScalarValue& lhs, const ScalarValue& rhs) {
    return static_cast<std::int64_t>(scalar_to_double(lhs) <= scalar_to_double(rhs));
}

ScalarValue compare_greater_scalars(const ScalarValue& lhs, const ScalarValue& rhs) {
    return static_cast<std::int64_t>(scalar_to_double(lhs) > scalar_to_double(rhs));
}

ScalarValue compare_greater_equal_scalars(const ScalarValue& lhs, const ScalarValue& rhs) {
    return static_cast<std::int64_t>(scalar_to_double(lhs) >= scalar_to_double(rhs));
}

ScalarValue negate_scalar(const ScalarValue& value) {
    if (const auto integer = try_get_int64(value); integer.has_value()) {
        if (*integer == std::numeric_limits<std::int64_t>::min()) {
            return require_finite_result(-static_cast<double>(*integer));
        }
        return -*integer;
    }

    return require_finite_result(-scalar_to_double(value));
}

ScalarValue bitwise_not_scalar(const ScalarValue& value) {
    return ~require_integer_operand(value);
}

ScalarValue apply_binary_operator(BinaryOperator op, const ScalarValue& lhs,
                                  const ScalarValue& rhs) {
    switch (op) {
    case BinaryOperator::add:
        return add_scalars(lhs, rhs);
    case BinaryOperator::subtract:
        return subtract_scalars(lhs, rhs);
    case BinaryOperator::equal:
        return compare_equal_scalars(lhs, rhs);
    case BinaryOperator::less:
        return compare_less_scalars(lhs, rhs);
    case BinaryOperator::less_equal:
        return compare_less_equal_scalars(lhs, rhs);
    case BinaryOperator::greater:
        return compare_greater_scalars(lhs, rhs);
    case BinaryOperator::greater_equal:
        return compare_greater_equal_scalars(lhs, rhs);
    case BinaryOperator::multiply:
        return multiply_scalars(lhs, rhs);
    case BinaryOperator::divide:
        return divide_scalars(lhs, rhs);
    case BinaryOperator::modulo:
        return modulo_scalars(lhs, rhs);
    case BinaryOperator::power:
        return power_scalars(lhs, rhs);
    case BinaryOperator::bitwise_and: {
        return require_integer_operand(lhs) & require_integer_operand(rhs);
    }
    case BinaryOperator::bitwise_or: {
        return require_integer_operand(lhs) | require_integer_operand(rhs);
    }
    }

    throw EvaluationError("unknown binary operator");
}

}  // namespace console_calc
