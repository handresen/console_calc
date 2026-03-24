#pragma once

#include "console_calc/builtin_function.h"
#include "console_calc/expression_ast.h"
#include "console_calc/scalar_value.h"
#include "console_calc/value.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace console_calc::detail {

[[nodiscard]] Value evaluate_expression_with_placeholder(
    const Expression& expression, const std::optional<ScalarValue>& placeholder_value);

[[nodiscard]] ScalarValue require_scalar_value(const Value& value);
[[nodiscard]] ScalarValue require_scalar_or_singleton_list_value(const Value& value);
[[nodiscard]] ListValue require_list(const Value& value);
[[nodiscard]] PositionListValue require_position_list(const Value& value);
[[nodiscard]] PositionValue require_position(const Value& value);
[[nodiscard]] std::size_t require_list_index(const ScalarValue& value);
[[nodiscard]] std::size_t require_positive_list_step(const ScalarValue& value);
[[nodiscard]] std::int64_t require_integer_operand(const ScalarValue& value);
[[nodiscard]] double require_finite_result(double value);

[[nodiscard]] Value evaluate_builtin_function(Function function, std::span<const Value> arguments);
[[nodiscard]] Value evaluate_map_call(const MapCall& node,
                                      const std::optional<ScalarValue>& placeholder_value);
[[nodiscard]] Value evaluate_list_where_call(const ListWhereCall& node,
                                             const std::optional<ScalarValue>& placeholder_value);
[[nodiscard]] Value evaluate_guard_call(const GuardCall& node,
                                        const std::optional<ScalarValue>& placeholder_value);
[[nodiscard]] Value evaluate_reduce_call(const ReduceCall& node,
                                         const std::optional<ScalarValue>& placeholder_value);
[[nodiscard]] Value evaluate_timed_loop_call(const TimedLoopCall& node,
                                             const std::optional<ScalarValue>& placeholder_value);
[[nodiscard]] Value evaluate_fill_call(const FillCall& node,
                                       const std::optional<ScalarValue>& placeholder_value);

}  // namespace console_calc::detail
