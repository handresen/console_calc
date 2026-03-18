#include "expression_evaluator.h"

#include "console_calc/builtin_function.h"
#include "console_calc/expression_error.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <type_traits>
#include <variant>

namespace console_calc {

namespace {

[[nodiscard]] double require_scalar(const Value& value) {
    if (const auto* scalar = std::get_if<double>(&value)) {
        return *scalar;
    }

    throw EvaluationError("list value cannot be used as a scalar");
}

[[nodiscard]] double require_scalar_or_singleton_list(const Value& value) {
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

[[nodiscard]] std::size_t require_list_index(double value) {
    if (!std::isfinite(value)) {
        throw EvaluationError("list count must be a non-negative integer");
    }

    double integral_part = 0.0;
    if (std::modf(value, &integral_part) != 0.0 || integral_part < 0.0) {
        throw EvaluationError("list count must be a non-negative integer");
    }

    return static_cast<std::size_t>(integral_part);
}

[[nodiscard]] std::int64_t require_integer_operand(double value) {
    if (!std::isfinite(value)) {
        throw EvaluationError("bitwise operators require 64-bit integer operands");
    }

    double integral_part = 0.0;
    if (std::modf(value, &integral_part) != 0.0) {
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
    case Function::pow:
    case Function::sum:
    case Function::len:
    case Function::product:
    case Function::avg:
    case Function::min:
    case Function::max:
    case Function::first:
    case Function::drop:
    case Function::map:
        break;
    }

    throw EvaluationError("function cannot be applied elementwise");
}

[[nodiscard]] Value evaluate_builtin_function(Function function, std::span<const Value> arguments) {
    switch (function) {
    case Function::sin:
    case Function::cos:
    case Function::tan:
    case Function::sind:
    case Function::cosd:
    case Function::tand:
        return evaluate_unary_scalar_builtin(
            function, require_scalar_or_singleton_list(arguments[0]));
    case Function::pow:
        return require_finite_result(
            std::pow(require_scalar_or_singleton_list(arguments[0]),
                     require_scalar_or_singleton_list(arguments[1])));
    case Function::sum: {
        const ListValue values = require_list(arguments[0]);
        double total = 0.0;
        for (const double value : values) {
            total = require_finite_result(total + value);
        }
        return total;
    }
    case Function::len: {
        const ListValue values = require_list(arguments[0]);
        return static_cast<double>(values.size());
    }
    case Function::product: {
        const ListValue values = require_list(arguments[0]);
        double total = 1.0;
        for (const double value : values) {
            total = require_finite_result(total * value);
        }
        return total;
    }
    case Function::avg: {
        const ListValue values = require_list(arguments[0]);
        if (values.empty()) {
            throw EvaluationError("avg() requires a non-empty list");
        }

        double total = 0.0;
        for (const double value : values) {
            total = require_finite_result(total + value);
        }
        return require_finite_result(total / static_cast<double>(values.size()));
    }
    case Function::min: {
        const ListValue values = require_list(arguments[0]);
        if (values.empty()) {
            throw EvaluationError("min() requires a non-empty list");
        }

        double result = values.front();
        for (std::size_t index = 1; index < values.size(); ++index) {
            result = std::min(result, values[index]);
        }
        return result;
    }
    case Function::max: {
        const ListValue values = require_list(arguments[0]);
        if (values.empty()) {
            throw EvaluationError("max() requires a non-empty list");
        }

        double result = values.front();
        for (std::size_t index = 1; index < values.size(); ++index) {
            result = std::max(result, values[index]);
        }
        return result;
    }
    case Function::first: {
        const std::size_t count = require_list_index(require_scalar_or_singleton_list(arguments[0]));
        const ListValue values = require_list(arguments[1]);
        const std::size_t result_size = std::min(count, values.size());
        return ListValue(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(result_size));
    }
    case Function::drop: {
        const std::size_t count = require_list_index(require_scalar_or_singleton_list(arguments[0]));
        const ListValue values = require_list(arguments[1]);
        const std::size_t skip = std::min(count, values.size());
        return ListValue(values.begin() + static_cast<std::ptrdiff_t>(skip), values.end());
    }
    case Function::map:
        break;
    }

    throw EvaluationError("unknown function");
}

}  // namespace

Value evaluate_expression(const Expression& expression) {
    return std::visit(
        [](const auto& node) -> Value {
            using Node = std::decay_t<decltype(node)>;

            if constexpr (std::is_same_v<Node, NumberLiteral>) {
                return node.value;
            } else if constexpr (std::is_same_v<Node, UnaryExpression>) {
                return require_finite_result(
                    -require_scalar_or_singleton_list(evaluate_expression(*node.operand)));
            } else if constexpr (std::is_same_v<Node, ListLiteral>) {
                ListValue values;
                values.reserve(node.elements.size());
                for (const auto& element : node.elements) {
                    values.push_back(require_scalar(evaluate_expression(*element)));
                }
                return values;
            } else if constexpr (std::is_same_v<Node, FunctionCall>) {
                std::vector<Value> arguments;
                arguments.reserve(node.arguments.size());
                for (const auto& argument : node.arguments) {
                    arguments.push_back(evaluate_expression(*argument));
                }
                return evaluate_builtin_function(node.function, arguments);
            } else if constexpr (std::is_same_v<Node, MapCall>) {
                const BuiltinFunctionInfo& mapped_info = builtin_function_info(node.mapped_function);
                if (!mapped_info.mappable || mapped_info.arity != 1 ||
                    mapped_info.category != BuiltinFunctionCategory::scalar) {
                    throw EvaluationError("map() requires a unary scalar builtin function");
                }

                const ListValue input_values = require_list(evaluate_expression(*node.list_argument));
                ListValue mapped_values;
                mapped_values.reserve(input_values.size());
                for (const double value : input_values) {
                    const Value scalar_value = value;
                    mapped_values.push_back(require_scalar(
                        evaluate_builtin_function(node.mapped_function, std::span{&scalar_value, 1})));
                }
                return mapped_values;
            } else {
                const double lhs = require_scalar_or_singleton_list(evaluate_expression(*node.left));
                const double rhs = require_scalar_or_singleton_list(evaluate_expression(*node.right));

                switch (node.op) {
                case BinaryOperator::add:
                    return require_finite_result(lhs + rhs);
                case BinaryOperator::subtract:
                    return require_finite_result(lhs - rhs);
                case BinaryOperator::multiply:
                    return require_finite_result(lhs * rhs);
                case BinaryOperator::divide:
                    if (rhs == 0.0) {
                        throw EvaluationError("division by zero");
                    }

                    return require_finite_result(lhs / rhs);
                case BinaryOperator::modulo:
                    if (rhs == 0.0) {
                        throw EvaluationError("modulo by zero");
                    }

                    return require_finite_result(std::fmod(lhs, rhs));
                case BinaryOperator::power:
                    return require_finite_result(std::pow(lhs, rhs));
                case BinaryOperator::bitwise_and:
                    return static_cast<double>(require_integer_operand(lhs) &
                                               require_integer_operand(rhs));
                case BinaryOperator::bitwise_or:
                    return static_cast<double>(require_integer_operand(lhs) |
                                               require_integer_operand(rhs));
                }

                throw EvaluationError("unknown binary operator");
            }
        },
        expression.node);
}

double evaluate_scalar_expression(const Expression& expression) {
    return require_scalar_or_singleton_list(evaluate_expression(expression));
}

}  // namespace console_calc
