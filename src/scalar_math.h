#pragma once

#include <cstdint>
#include <optional>

#include "console_calc/expression_ast.h"
#include "console_calc/value.h"

namespace console_calc {

[[nodiscard]] std::optional<std::int64_t> checked_add_int64(std::int64_t lhs, std::int64_t rhs);
[[nodiscard]] std::optional<std::int64_t> checked_subtract_int64(std::int64_t lhs,
                                                                 std::int64_t rhs);
[[nodiscard]] std::optional<std::int64_t> checked_multiply_int64(std::int64_t lhs,
                                                                 std::int64_t rhs);
[[nodiscard]] std::optional<std::int64_t> checked_power_int64(std::int64_t base,
                                                              std::int64_t exponent);

[[nodiscard]] ScalarValue add_scalars(const ScalarValue& lhs, const ScalarValue& rhs);
[[nodiscard]] ScalarValue subtract_scalars(const ScalarValue& lhs, const ScalarValue& rhs);
[[nodiscard]] ScalarValue multiply_scalars(const ScalarValue& lhs, const ScalarValue& rhs);
[[nodiscard]] ScalarValue divide_scalars(const ScalarValue& lhs, const ScalarValue& rhs);
[[nodiscard]] ScalarValue modulo_scalars(const ScalarValue& lhs, const ScalarValue& rhs);
[[nodiscard]] ScalarValue power_scalars(const ScalarValue& lhs, const ScalarValue& rhs);
[[nodiscard]] ScalarValue compare_equal_scalars(const ScalarValue& lhs, const ScalarValue& rhs);
[[nodiscard]] ScalarValue compare_less_scalars(const ScalarValue& lhs, const ScalarValue& rhs);
[[nodiscard]] ScalarValue compare_less_equal_scalars(const ScalarValue& lhs, const ScalarValue& rhs);
[[nodiscard]] ScalarValue compare_greater_scalars(const ScalarValue& lhs, const ScalarValue& rhs);
[[nodiscard]] ScalarValue compare_greater_equal_scalars(const ScalarValue& lhs,
                                                        const ScalarValue& rhs);
[[nodiscard]] ScalarValue negate_scalar(const ScalarValue& value);
[[nodiscard]] ScalarValue apply_binary_operator(BinaryOperator op, const ScalarValue& lhs,
                                                const ScalarValue& rhs);

}  // namespace console_calc
