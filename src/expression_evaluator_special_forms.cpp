#include "expression_evaluator_internal.h"

#include "console_calc/expression_error.h"

#include <chrono>
#include <optional>

#include "scalar_math.h"

namespace console_calc::detail {

Value evaluate_map_call(const MapCall& node,
                        const std::optional<ScalarValue>& placeholder_value) {
    const ListValue input_values =
        require_list(evaluate_expression_with_placeholder(*node.list_argument, placeholder_value));

    std::size_t start = 0;
    std::size_t step = 1;
    std::optional<std::size_t> count;
    if (node.start_argument != nullptr) {
        start = require_list_index(require_scalar_or_singleton_list_value(
            evaluate_expression_with_placeholder(*node.start_argument, placeholder_value)));
    }
    if (node.step_argument != nullptr) {
        step = require_positive_list_step(require_scalar_or_singleton_list_value(
            evaluate_expression_with_placeholder(*node.step_argument, placeholder_value)));
    }
    if (node.count_argument != nullptr) {
        count = require_list_index(require_scalar_or_singleton_list_value(
            evaluate_expression_with_placeholder(*node.count_argument, placeholder_value)));
    }

    start = std::min(start, input_values.size());
    if (start >= input_values.size() || (count.has_value() && *count == 0U)) {
        return node.preserve_unmapped ? Value{input_values} : Value{ListValue{}};
    }

    ListValue mapped_values = node.preserve_unmapped ? input_values : ListValue{};
    if (!node.preserve_unmapped) {
        if (count.has_value()) {
            mapped_values.reserve(*count);
        } else {
            mapped_values.reserve(((input_values.size() - start) + step - 1U) / step);
        }
    }

    std::size_t emitted = 0;
    for (std::size_t index = start; index < input_values.size(); index += step) {
        if (count.has_value() && emitted >= *count) {
            break;
        }
        const ScalarValue mapped_value = require_scalar_value(
            evaluate_expression_with_placeholder(*node.mapped_expression, input_values[index]));
        if (node.preserve_unmapped) {
            mapped_values[index] = mapped_value;
        } else {
            mapped_values.push_back(mapped_value);
        }
        ++emitted;
    }
    return mapped_values;
}

Value evaluate_list_where_call(const ListWhereCall& node,
                               const std::optional<ScalarValue>& placeholder_value) {
    const ListValue input_values =
        require_list(evaluate_expression_with_placeholder(*node.list_argument, placeholder_value));

    ListValue filtered_values;
    filtered_values.reserve(input_values.size());
    for (const auto& input_value : input_values) {
        const ScalarValue predicate_value = require_scalar_value(
            evaluate_expression_with_placeholder(*node.predicate_expression, input_value));
        if (scalar_to_double(predicate_value) != 0.0) {
            filtered_values.push_back(input_value);
        }
    }

    return filtered_values;
}

Value evaluate_guard_call(const GuardCall& node,
                          const std::optional<ScalarValue>& placeholder_value) {
    try {
        return evaluate_expression_with_placeholder(*node.guarded_expression, placeholder_value);
    } catch (const std::invalid_argument&) {
        return evaluate_expression_with_placeholder(*node.fallback_expression, placeholder_value);
    }
}

Value evaluate_reduce_call(const ReduceCall& node,
                           const std::optional<ScalarValue>& placeholder_value) {
    const ListValue input_values =
        require_list(evaluate_expression_with_placeholder(*node.list_argument, placeholder_value));
    if (input_values.empty()) {
        throw EvaluationError("reduce() requires a non-empty list");
    }

    ScalarValue reduced = input_values.front();
    for (std::size_t index = 1; index < input_values.size(); ++index) {
        reduced = apply_binary_operator(node.reduction_operator, reduced, input_values[index]);
    }
    return to_value(reduced);
}

Value evaluate_timed_loop_call(const TimedLoopCall& node,
                               const std::optional<ScalarValue>& placeholder_value) {
    const std::size_t iteration_count = require_list_index(
        require_scalar_or_singleton_list_value(
            evaluate_expression_with_placeholder(*node.iteration_count, placeholder_value)));

    const auto started_at = std::chrono::steady_clock::now();
    for (std::size_t index = 0; index < iteration_count; ++index) {
        (void)evaluate_expression_with_placeholder(*node.loop_expression, placeholder_value);
    }
    const auto elapsed =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - started_at).count();
    return require_finite_result(elapsed);
}

Value evaluate_fill_call(const FillCall& node,
                         const std::optional<ScalarValue>& placeholder_value) {
    const std::size_t iteration_count = require_list_index(
        require_scalar_or_singleton_list_value(
            evaluate_expression_with_placeholder(*node.iteration_count, placeholder_value)));

    if (iteration_count == 0) {
        return ListValue{};
    }

    const Value first_value =
        evaluate_expression_with_placeholder(*node.fill_expression, placeholder_value);
    if (const auto* scalar = std::get_if<std::int64_t>(&first_value)) {
        ListValue values;
        values.reserve(iteration_count);
        values.push_back(*scalar);
        for (std::size_t index = 1; index < iteration_count; ++index) {
            values.push_back(require_scalar_value(
                evaluate_expression_with_placeholder(*node.fill_expression, placeholder_value)));
        }
        return values;
    }
    if (const auto* scalar = std::get_if<double>(&first_value)) {
        ListValue values;
        values.reserve(iteration_count);
        values.push_back(*scalar);
        for (std::size_t index = 1; index < iteration_count; ++index) {
            values.push_back(require_scalar_value(
                evaluate_expression_with_placeholder(*node.fill_expression, placeholder_value)));
        }
        return values;
    }
    if (const auto* position = std::get_if<PositionValue>(&first_value)) {
        PositionListValue values;
        values.reserve(iteration_count);
        values.push_back(*position);
        for (std::size_t index = 1; index < iteration_count; ++index) {
            values.push_back(require_position(
                evaluate_expression_with_placeholder(*node.fill_expression, placeholder_value)));
        }
        return values;
    }

    throw EvaluationError("fill() requires a scalar or position expression");
}

}  // namespace console_calc::detail
