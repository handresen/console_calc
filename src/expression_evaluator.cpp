#include "expression_evaluator.h"

#include "expression_evaluator_internal.h"

#include "console_calc/builtin_function.h"
#include "console_calc/expression_error.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <variant>

#include "scalar_math.h"

namespace console_calc {

namespace detail {

ScalarValue require_scalar_value(const Value& value) {
    if (const auto* integer = std::get_if<std::int64_t>(&value)) {
        return *integer;
    }
    if (const auto* scalar = std::get_if<double>(&value)) {
        return *scalar;
    }
    if (std::holds_alternative<PositionValue>(value)) {
        throw EvaluationError("position value cannot be used as a scalar");
    }
    if (std::holds_alternative<PositionListValue>(value)) {
        throw EvaluationError("position list value cannot be used as a scalar");
    }

    throw EvaluationError("list value cannot be used as a scalar");
}

ScalarValue require_scalar_or_singleton_list_value(const Value& value) {
    if (const auto* integer = std::get_if<std::int64_t>(&value)) {
        return *integer;
    }
    if (const auto* scalar = std::get_if<double>(&value)) {
        return *scalar;
    }
    if (const auto* list = std::get_if<ListValue>(&value)) {
        if (list->size() == 1) {
            return (*list)[0];
        }
        throw EvaluationError("list value cannot be used as a scalar");
    }
    if (std::holds_alternative<PositionValue>(value)) {
        throw EvaluationError("position value cannot be used as a scalar");
    }
    if (std::holds_alternative<PositionListValue>(value)) {
        throw EvaluationError("position list value cannot be used as a scalar");
    }

    throw EvaluationError("scalar value required");
}

ListValue require_list(const Value& value) {
    if (const auto* list = std::get_if<ListValue>(&value)) {
        return *list;
    }
    if (std::holds_alternative<PositionListValue>(value)) {
        throw EvaluationError("scalar list value required");
    }

    throw EvaluationError("list value required");
}

PositionListValue require_position_list(const Value& value) {
    if (const auto* list = std::get_if<PositionListValue>(&value)) {
        return *list;
    }

    throw EvaluationError("position list value required");
}

PositionValue require_position(const Value& value) {
    if (const auto* position = std::get_if<PositionValue>(&value)) {
        return *position;
    }

    throw EvaluationError("position value required");
}

std::size_t require_list_index(const ScalarValue& value) {
    if (const auto* integer = std::get_if<std::int64_t>(&value)) {
        if (*integer < 0) {
            throw EvaluationError("list count must be a non-negative integer");
        }
        return static_cast<std::size_t>(*integer);
    }

    const double numeric_value = std::get<double>(value);
    if (!std::isfinite(numeric_value)) {
        throw EvaluationError("list count must be a non-negative integer");
    }

    double integral_part = 0.0;
    if (std::modf(numeric_value, &integral_part) != 0.0 || integral_part < 0.0) {
        throw EvaluationError("list count must be a non-negative integer");
    }

    return static_cast<std::size_t>(integral_part);
}

std::size_t require_positive_list_step(const ScalarValue& value) {
    const std::size_t step = require_list_index(value);
    if (step == 0) {
        throw EvaluationError("map() step must be a positive integer");
    }
    return step;
}

std::int64_t require_integer_operand(const ScalarValue& value) {
    if (const auto* integer = std::get_if<std::int64_t>(&value)) {
        return *integer;
    }

    const double numeric_value = std::get<double>(value);
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

double require_finite_result(double value) {
    if (!std::isfinite(value)) {
        throw EvaluationError("expression produced a non-finite result");
    }

    return value;
}

[[nodiscard]] Value evaluate_homogeneous_list_literal(
    std::span<const std::unique_ptr<Expression>> elements,
    const std::optional<ScalarValue>& placeholder_value) {
    if (elements.empty()) {
        return ListValue{};
    }

    const Value first_value =
        evaluate_expression_with_placeholder(*elements.front(), placeholder_value);
    if (const auto* scalar = std::get_if<std::int64_t>(&first_value)) {
        ListValue values;
        values.reserve(elements.size());
        values.push_back(*scalar);
        for (std::size_t index = 1; index < elements.size(); ++index) {
            values.push_back(require_scalar_value(
                evaluate_expression_with_placeholder(*elements[index], placeholder_value)));
        }
        return values;
    }
    if (const auto* scalar = std::get_if<double>(&first_value)) {
        ListValue values;
        values.reserve(elements.size());
        values.push_back(*scalar);
        for (std::size_t index = 1; index < elements.size(); ++index) {
            values.push_back(require_scalar_value(
                evaluate_expression_with_placeholder(*elements[index], placeholder_value)));
        }
        return values;
    }
    if (const auto* position = std::get_if<PositionValue>(&first_value)) {
        PositionListValue values;
        values.reserve(elements.size());
        values.push_back(*position);
        for (std::size_t index = 1; index < elements.size(); ++index) {
            values.push_back(require_position(
                evaluate_expression_with_placeholder(*elements[index], placeholder_value)));
        }
        return values;
    }

    throw EvaluationError("nested lists are not supported");
}

[[nodiscard]] Value evaluate_binary_expression(const BinaryExpression& node,
                                               const std::optional<ScalarValue>& placeholder_value) {
    const ScalarValue lhs = require_scalar_or_singleton_list_value(
        evaluate_expression_with_placeholder(*node.left, placeholder_value));
    const ScalarValue rhs = require_scalar_or_singleton_list_value(
        evaluate_expression_with_placeholder(*node.right, placeholder_value));
    return to_value(apply_binary_operator(node.op, lhs, rhs));
}

Value evaluate_expression_with_placeholder(const Expression& expression,
                                           const std::optional<ScalarValue>& placeholder_value) {
    return std::visit(
        [&](const auto& node) -> Value {
            using Node = std::decay_t<decltype(node)>;

            if constexpr (std::is_same_v<Node, NumberLiteral>) {
                return to_value(node.value);
            } else if constexpr (std::is_same_v<Node, PlaceholderExpression>) {
                if (!placeholder_value.has_value()) {
                    throw EvaluationError(
                        "placeholder '_' can only be used inside map(), map_at(), or list_where()");
                }
                return to_value(*placeholder_value);
            } else if constexpr (std::is_same_v<Node, UnaryExpression>) {
                return to_value(negate_scalar(
                    require_scalar_or_singleton_list_value(
                        evaluate_expression_with_placeholder(*node.operand, placeholder_value))));
            } else if constexpr (std::is_same_v<Node, ListLiteral>) {
                return evaluate_homogeneous_list_literal(node.elements, placeholder_value);
            } else if constexpr (std::is_same_v<Node, FunctionCall>) {
                std::vector<Value> arguments;
                arguments.reserve(node.arguments.size());
                for (const auto& argument : node.arguments) {
                    arguments.push_back(
                        evaluate_expression_with_placeholder(*argument, placeholder_value));
                }
                return evaluate_builtin_function(node.function, arguments);
            } else if constexpr (std::is_same_v<Node, MapCall>) {
                return evaluate_map_call(node, placeholder_value);
            } else if constexpr (std::is_same_v<Node, ListWhereCall>) {
                return evaluate_list_where_call(node, placeholder_value);
            } else if constexpr (std::is_same_v<Node, GuardCall>) {
                return evaluate_guard_call(node, placeholder_value);
            } else if constexpr (std::is_same_v<Node, ReduceCall>) {
                return evaluate_reduce_call(node, placeholder_value);
            } else if constexpr (std::is_same_v<Node, TimedLoopCall>) {
                return evaluate_timed_loop_call(node, placeholder_value);
            } else if constexpr (std::is_same_v<Node, FillCall>) {
                return evaluate_fill_call(node, placeholder_value);
            } else {
                return evaluate_binary_expression(node, placeholder_value);
            }
        },
        expression.node);
}

}  // namespace detail

Value evaluate_expression(const Expression& expression) {
    return detail::evaluate_expression_with_placeholder(expression, std::nullopt);
}

double evaluate_scalar_expression(const Expression& expression) {
    return scalar_to_double(
        detail::require_scalar_or_singleton_list_value(evaluate_expression(expression)));
}

}  // namespace console_calc
