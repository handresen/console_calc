#include "expression_evaluator.h"

#include "console_calc/builtin_function.h"
#include "console_calc/expression_error.h"
#include "console_calc/scalar_value.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <span>
#include <type_traits>
#include <variant>

#include "scalar_math.h"

namespace console_calc {

namespace {

[[nodiscard]] ScalarValue require_scalar_value(const Value& value) {
    if (const auto* integer = std::get_if<std::int64_t>(&value)) {
        return *integer;
    }
    if (const auto* scalar = std::get_if<double>(&value)) {
        return *scalar;
    }

    throw EvaluationError("list value cannot be used as a scalar");
}

[[nodiscard]] ScalarValue require_scalar_or_singleton_list_value(const Value& value) {
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
    }

    throw EvaluationError("list value cannot be used as a scalar");
}

[[nodiscard]] ListValue require_list(const Value& value) {
    if (const auto* list = std::get_if<ListValue>(&value)) {
        return *list;
    }

    throw EvaluationError("list value required");
}

[[nodiscard]] std::size_t require_list_index(const ScalarValue& value) {
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

[[nodiscard]] std::int64_t require_integer_operand(const ScalarValue& value) {
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

[[nodiscard]] double require_finite_result(double value) {
    if (!std::isfinite(value)) {
        throw EvaluationError("expression produced a non-finite result");
    }

    return value;
}

[[nodiscard]] double degrees_to_radians(double value) {
    return value * std::numbers::pi_v<double> / 180.0;
}

[[nodiscard]] double evaluate_unary_scalar_builtin(Function function, double argument) {
    switch (function) {
    case Function::abs:
        return require_finite_result(std::fabs(argument));
    case Function::sin:
        return require_finite_result(std::sin(argument));
    case Function::cos:
        return require_finite_result(std::cos(argument));
    case Function::tan:
        return require_finite_result(std::tan(argument));
    case Function::sind:
        return require_finite_result(std::sin(degrees_to_radians(argument)));
    case Function::cosd:
        return require_finite_result(std::cos(degrees_to_radians(argument)));
    case Function::tand:
        return require_finite_result(std::tan(degrees_to_radians(argument)));
    case Function::sqrt:
        if (argument < 0.0) {
            throw EvaluationError("sqrt() requires a non-negative input");
        }
        return require_finite_result(std::sqrt(argument));
    case Function::pow:
    case Function::sum:
    case Function::len:
    case Function::product:
    case Function::avg:
    case Function::min:
    case Function::max:
    case Function::first:
    case Function::drop:
    case Function::list_add:
    case Function::list_sub:
    case Function::list_div:
    case Function::list_mul:
    case Function::reduce:
    case Function::map:
    case Function::range:
    case Function::geom:
    case Function::repeat:
    case Function::linspace:
    case Function::powers:
        break;
    }

    throw EvaluationError("function cannot be applied elementwise");
}

template <typename Operation>
[[nodiscard]] Value evaluate_pairwise_list_builtin(std::span<const Value> arguments,
                                                   std::string_view function_name,
                                                   Operation operation) {
    const ListValue lhs = require_list(arguments[0]);
    const ListValue rhs = require_list(arguments[1]);
    if (lhs.size() != rhs.size()) {
        throw EvaluationError(std::string(function_name) + "() requires lists of equal length");
    }

    ListValue values;
    values.reserve(lhs.size());
    for (std::size_t index = 0; index < lhs.size(); ++index) {
        values.push_back(operation(lhs[index], rhs[index]));
    }
    return values;
}

[[nodiscard]] Value evaluate_scalar_builtin(Function function, std::span<const Value> arguments) {
    switch (function) {
    case Function::abs: {
        const ScalarValue value = require_scalar_or_singleton_list_value(arguments[0]);
        if (const auto* integer = std::get_if<std::int64_t>(&value)) {
            if (*integer == std::numeric_limits<std::int64_t>::min()) {
                return require_finite_result(std::fabs(static_cast<double>(*integer)));
            }
            return *integer < 0 ? to_value(-*integer) : to_value(*integer);
        }
        return require_finite_result(std::fabs(std::get<double>(value)));
    }
    case Function::sin:
    case Function::cos:
    case Function::tan:
    case Function::sind:
    case Function::cosd:
    case Function::tand:
    case Function::sqrt:
        return evaluate_unary_scalar_builtin(
            function, scalar_to_double(require_scalar_or_singleton_list_value(arguments[0])));
    case Function::pow:
        return to_value(power_scalars(
            require_scalar_or_singleton_list_value(arguments[0]),
            require_scalar_or_singleton_list_value(arguments[1])));
    case Function::sum:
    case Function::len:
    case Function::product:
    case Function::avg:
    case Function::min:
    case Function::max:
    case Function::first:
    case Function::drop:
    case Function::list_add:
    case Function::list_sub:
    case Function::list_div:
    case Function::list_mul:
    case Function::reduce:
    case Function::map:
    case Function::range:
    case Function::geom:
    case Function::repeat:
    case Function::linspace:
    case Function::powers:
        break;
    }

    throw EvaluationError("unknown scalar builtin");
}

[[nodiscard]] Value evaluate_list_builtin(Function function, std::span<const Value> arguments) {
    switch (function) {
    case Function::sum: {
        const ListValue values = require_list(arguments[0]);
        ScalarValue total = std::int64_t{0};
        for (const auto& value : values) {
            total = add_scalars(total, value);
        }
        return to_value(total);
    }
    case Function::len: {
        const ListValue values = require_list(arguments[0]);
        return static_cast<std::int64_t>(values.size());
    }
    case Function::product: {
        const ListValue values = require_list(arguments[0]);
        ScalarValue total = std::int64_t{1};
        for (const auto& value : values) {
            total = multiply_scalars(total, value);
        }
        return to_value(total);
    }
    case Function::avg: {
        const ListValue values = require_list(arguments[0]);
        if (values.empty()) {
            throw EvaluationError("avg() requires a non-empty list");
        }

        double total = 0.0;
        for (const auto& value : values) {
            total = require_finite_result(total + scalar_to_double(value));
        }
        return require_finite_result(total / static_cast<double>(values.size()));
    }
    case Function::min: {
        const ListValue values = require_list(arguments[0]);
        if (values.empty()) {
            throw EvaluationError("min() requires a non-empty list");
        }

        ScalarValue result = values.front();
        for (std::size_t index = 1; index < values.size(); ++index) {
            if (scalar_to_double(values[index]) < scalar_to_double(result)) {
                result = values[index];
            }
        }
        return to_value(result);
    }
    case Function::max: {
        const ListValue values = require_list(arguments[0]);
        if (values.empty()) {
            throw EvaluationError("max() requires a non-empty list");
        }

        ScalarValue result = values.front();
        for (std::size_t index = 1; index < values.size(); ++index) {
            if (scalar_to_double(values[index]) > scalar_to_double(result)) {
                result = values[index];
            }
        }
        return to_value(result);
    }
    case Function::first: {
        const std::size_t count =
            require_list_index(require_scalar_or_singleton_list_value(arguments[0]));
        const ListValue values = require_list(arguments[1]);
        const std::size_t result_size = std::min(count, values.size());
        return ListValue(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(result_size));
    }
    case Function::drop: {
        const std::size_t count =
            require_list_index(require_scalar_or_singleton_list_value(arguments[0]));
        const ListValue values = require_list(arguments[1]);
        const std::size_t skip = std::min(count, values.size());
        return ListValue(values.begin() + static_cast<std::ptrdiff_t>(skip), values.end());
    }
    case Function::list_add:
        return evaluate_pairwise_list_builtin(arguments, "list_add", add_scalars);
    case Function::list_sub:
        return evaluate_pairwise_list_builtin(arguments, "list_sub", subtract_scalars);
    case Function::list_div:
        return evaluate_pairwise_list_builtin(arguments, "list_div", divide_scalars);
    case Function::list_mul:
        return evaluate_pairwise_list_builtin(arguments, "list_mul", multiply_scalars);
    case Function::map:
    case Function::reduce:
        break;
    case Function::abs:
    case Function::sin:
    case Function::cos:
    case Function::tan:
    case Function::sind:
    case Function::cosd:
    case Function::tand:
    case Function::sqrt:
    case Function::pow:
    case Function::range:
    case Function::geom:
    case Function::repeat:
    case Function::linspace:
    case Function::powers:
        break;
    }

    throw EvaluationError("unknown list builtin");
}

[[nodiscard]] Value evaluate_list_generation_builtin(Function function,
                                                     std::span<const Value> arguments) {
    switch (function) {
    case Function::range: {
        const ScalarValue start = require_scalar_or_singleton_list_value(arguments[0]);
        const std::size_t count =
            require_list_index(require_scalar_or_singleton_list_value(arguments[1]));
        const ScalarValue step =
            arguments.size() == 3 ? require_scalar_or_singleton_list_value(arguments[2])
                                  : ScalarValue{std::int64_t{1}};

        ListValue values;
        values.reserve(count);
        ScalarValue current = start;
        for (std::size_t index = 0; index < count; ++index) {
            values.push_back(current);
            current = add_scalars(current, step);
        }
        return values;
    }
    case Function::geom: {
        const ScalarValue start = require_scalar_or_singleton_list_value(arguments[0]);
        const std::size_t count =
            require_list_index(require_scalar_or_singleton_list_value(arguments[1]));
        const ScalarValue ratio =
            arguments.size() == 3 ? require_scalar_or_singleton_list_value(arguments[2])
                                  : ScalarValue{std::int64_t{2}};

        ListValue values;
        values.reserve(count);
        ScalarValue current = start;
        for (std::size_t index = 0; index < count; ++index) {
            values.push_back(current);
            current = multiply_scalars(current, ratio);
        }
        return values;
    }
    case Function::repeat: {
        const ScalarValue value = require_scalar_or_singleton_list_value(arguments[0]);
        const std::size_t count =
            require_list_index(require_scalar_or_singleton_list_value(arguments[1]));
        return ListValue(count, value);
    }
    case Function::linspace: {
        const ScalarValue start = require_scalar_or_singleton_list_value(arguments[0]);
        const ScalarValue stop = require_scalar_or_singleton_list_value(arguments[1]);
        const std::size_t count =
            require_list_index(require_scalar_or_singleton_list_value(arguments[2]));

        ListValue values;
        values.reserve(count);
        if (count == 0) {
            return values;
        }

        values.push_back(start);
        if (count == 1) {
            return values;
        }

        const ScalarValue denominator = static_cast<std::int64_t>(count - 1);
        const ScalarValue step = divide_scalars(subtract_scalars(stop, start), denominator);
        ScalarValue current = start;
        for (std::size_t index = 1; index < count; ++index) {
            current = add_scalars(current, step);
            values.push_back(current);
        }
        return values;
    }
    case Function::powers: {
        const ScalarValue base = require_scalar_or_singleton_list_value(arguments[0]);
        const std::size_t count =
            require_list_index(require_scalar_or_singleton_list_value(arguments[1]));
        const ScalarValue start_exponent =
            arguments.size() == 3 ? require_scalar_or_singleton_list_value(arguments[2])
                                  : ScalarValue{std::int64_t{0}};

        ListValue values;
        values.reserve(count);
        for (std::size_t index = 0; index < count; ++index) {
            const ScalarValue exponent =
                add_scalars(start_exponent, static_cast<std::int64_t>(index));
            values.push_back(power_scalars(base, exponent));
        }
        return values;
    }
    case Function::abs:
    case Function::sin:
    case Function::cos:
    case Function::tan:
    case Function::sind:
    case Function::cosd:
    case Function::tand:
    case Function::sqrt:
    case Function::pow:
    case Function::sum:
    case Function::len:
    case Function::product:
    case Function::avg:
    case Function::min:
    case Function::max:
    case Function::first:
    case Function::drop:
    case Function::list_add:
    case Function::list_sub:
    case Function::list_div:
    case Function::list_mul:
    case Function::reduce:
    case Function::map:
        break;
    }

    throw EvaluationError("unknown list generation builtin");
}

[[nodiscard]] Value evaluate_builtin_function(Function function, std::span<const Value> arguments) {
    if (is_scalar_function(function)) {
        return evaluate_scalar_builtin(function, arguments);
    }

    if (builtin_function_info(function).category == BuiltinFunctionCategory::list) {
        return evaluate_list_builtin(function, arguments);
    }

    if (builtin_function_info(function).category == BuiltinFunctionCategory::list_generation) {
        return evaluate_list_generation_builtin(function, arguments);
    }

    throw EvaluationError("unknown function");
}

}  // namespace

Value evaluate_expression_with_placeholder(const Expression& expression,
                                           const std::optional<ScalarValue>& placeholder_value) {
    return std::visit(
        [&](const auto& node) -> Value {
            using Node = std::decay_t<decltype(node)>;

            if constexpr (std::is_same_v<Node, NumberLiteral>) {
                return to_value(node.value);
            } else if constexpr (std::is_same_v<Node, PlaceholderExpression>) {
                if (!placeholder_value.has_value()) {
                    throw EvaluationError("map placeholder '_' can only be used inside map()");
                }
                return to_value(*placeholder_value);
            } else if constexpr (std::is_same_v<Node, UnaryExpression>) {
                return to_value(negate_scalar(
                    require_scalar_or_singleton_list_value(
                        evaluate_expression_with_placeholder(*node.operand, placeholder_value))));
            } else if constexpr (std::is_same_v<Node, ListLiteral>) {
                ListValue values;
                values.reserve(node.elements.size());
                for (const auto& element : node.elements) {
                    values.push_back(require_scalar_value(
                        evaluate_expression_with_placeholder(*element, placeholder_value)));
                }
                return values;
            } else if constexpr (std::is_same_v<Node, FunctionCall>) {
                std::vector<Value> arguments;
                arguments.reserve(node.arguments.size());
                for (const auto& argument : node.arguments) {
                    arguments.push_back(
                        evaluate_expression_with_placeholder(*argument, placeholder_value));
                }
                return evaluate_builtin_function(node.function, arguments);
            } else if constexpr (std::is_same_v<Node, MapCall>) {
                const ListValue input_values =
                    require_list(evaluate_expression_with_placeholder(*node.list_argument,
                                                                     placeholder_value));
                ListValue mapped_values;
                mapped_values.reserve(input_values.size());
                for (const auto& value : input_values) {
                    if (node.mapped_function.has_value()) {
                        if (!is_mappable_unary_scalar_function(*node.mapped_function)) {
                            throw EvaluationError("map() requires a unary scalar builtin function");
                        }

                        const Value scalar_value = to_value(value);
                        mapped_values.push_back(require_scalar_value(evaluate_builtin_function(
                            *node.mapped_function, std::span{&scalar_value, 1})));
                    } else {
                        mapped_values.push_back(require_scalar_value(
                            evaluate_expression_with_placeholder(*node.mapped_expression, value)));
                    }
                }
                return mapped_values;
            } else if constexpr (std::is_same_v<Node, ReduceCall>) {
                const ListValue input_values =
                    require_list(evaluate_expression_with_placeholder(*node.list_argument,
                                                                     placeholder_value));
                if (input_values.empty()) {
                    throw EvaluationError("reduce() requires a non-empty list");
                }

                ScalarValue reduced = input_values.front();
                for (std::size_t index = 1; index < input_values.size(); ++index) {
                    reduced = apply_binary_operator(node.reduction_operator, reduced,
                                                    input_values[index]);
                }
                return to_value(reduced);
            } else {
                const ScalarValue lhs =
                    require_scalar_or_singleton_list_value(
                        evaluate_expression_with_placeholder(*node.left, placeholder_value));
                const ScalarValue rhs =
                    require_scalar_or_singleton_list_value(
                        evaluate_expression_with_placeholder(*node.right, placeholder_value));

                switch (node.op) {
                case BinaryOperator::add:
                    return to_value(add_scalars(lhs, rhs));
                case BinaryOperator::subtract:
                    return to_value(subtract_scalars(lhs, rhs));
                case BinaryOperator::multiply:
                    return to_value(multiply_scalars(lhs, rhs));
                case BinaryOperator::divide:
                    return to_value(divide_scalars(lhs, rhs));
                case BinaryOperator::modulo:
                    return to_value(modulo_scalars(lhs, rhs));
                case BinaryOperator::power:
                    return to_value(power_scalars(lhs, rhs));
                case BinaryOperator::bitwise_and:
                    return require_integer_operand(lhs) & require_integer_operand(rhs);
                case BinaryOperator::bitwise_or:
                    return require_integer_operand(lhs) | require_integer_operand(rhs);
                }

                throw EvaluationError("unknown binary operator");
            }
        },
        expression.node);
}

Value evaluate_expression(const Expression& expression) {
    return evaluate_expression_with_placeholder(expression, std::nullopt);
}

double evaluate_scalar_expression(const Expression& expression) {
    return scalar_to_double(require_scalar_or_singleton_list_value(evaluate_expression(expression)));
}

}  // namespace console_calc
