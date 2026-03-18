#include "expression_evaluator.h"

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

[[nodiscard]] ListValue require_list(const Value& value) {
    if (const auto* list = std::get_if<ListValue>(&value)) {
        return *list;
    }

    throw EvaluationError("list value required");
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

}  // namespace

Value evaluate_expression(const Expression& expression) {
    return std::visit(
        [](const auto& node) -> Value {
            using Node = std::decay_t<decltype(node)>;

            if constexpr (std::is_same_v<Node, NumberLiteral>) {
                return node.value;
            } else if constexpr (std::is_same_v<Node, UnaryExpression>) {
                return require_finite_result(-require_scalar(evaluate_expression(*node.operand)));
            } else if constexpr (std::is_same_v<Node, ListLiteral>) {
                ListValue values;
                values.reserve(node.elements.size());
                for (const auto& element : node.elements) {
                    values.push_back(require_scalar(evaluate_expression(*element)));
                }
                return values;
            } else if constexpr (std::is_same_v<Node, FunctionCall>) {
                switch (node.function) {
                case Function::sin:
                    return require_finite_result(
                        std::sin(require_scalar(evaluate_expression(*node.arguments[0]))));
                case Function::cos:
                    return require_finite_result(
                        std::cos(require_scalar(evaluate_expression(*node.arguments[0]))));
                case Function::tan:
                    return require_finite_result(
                        std::tan(require_scalar(evaluate_expression(*node.arguments[0]))));
                case Function::sind:
                    return require_finite_result(std::sin(degrees_to_radians(
                        require_scalar(evaluate_expression(*node.arguments[0])))));
                case Function::cosd:
                    return require_finite_result(std::cos(degrees_to_radians(
                        require_scalar(evaluate_expression(*node.arguments[0])))));
                case Function::tand:
                    return require_finite_result(std::tan(degrees_to_radians(
                        require_scalar(evaluate_expression(*node.arguments[0])))));
                case Function::pow:
                    return require_finite_result(std::pow(
                        require_scalar(evaluate_expression(*node.arguments[0])),
                        require_scalar(evaluate_expression(*node.arguments[1]))));
                case Function::sum: {
                    const ListValue values = require_list(evaluate_expression(*node.arguments[0]));
                    double total = 0.0;
                    for (const double value : values) {
                        total = require_finite_result(total + value);
                    }
                    return total;
                }
                }

                throw EvaluationError("unknown function");
            } else {
                const double lhs = require_scalar(evaluate_expression(*node.left));
                const double rhs = require_scalar(evaluate_expression(*node.right));

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
    return require_scalar(evaluate_expression(expression));
}

}  // namespace console_calc
