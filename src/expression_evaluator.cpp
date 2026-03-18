#include "expression_evaluator.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <variant>

namespace console_calc {

namespace {

[[nodiscard]] std::int64_t require_integer_operand(double value) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument("bitwise operators require 64-bit integer operands");
    }

    double integral_part = 0.0;
    if (std::modf(value, &integral_part) != 0.0) {
        throw std::invalid_argument("bitwise operators require 64-bit integer operands");
    }

    const long double integral_value = static_cast<long double>(integral_part);
    const long double min_value = static_cast<long double>(std::numeric_limits<std::int64_t>::min());
    const long double max_value = static_cast<long double>(std::numeric_limits<std::int64_t>::max());
    if (integral_value < min_value || integral_value > max_value) {
        throw std::invalid_argument("bitwise operators require 64-bit integer operands");
    }

    return static_cast<std::int64_t>(integral_part);
}

[[nodiscard]] double require_finite_result(double value) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument("expression produced a non-finite result");
    }

    return value;
}

}  // namespace

double evaluate_expression(const Expression& expression) {
    return std::visit(
        [](const auto& node) -> double {
            using Node = std::decay_t<decltype(node)>;

            if constexpr (std::is_same_v<Node, NumberLiteral>) {
                return node.value;
            } else {
                const double lhs = evaluate_expression(*node.left);
                const double rhs = evaluate_expression(*node.right);

                switch (node.op) {
                case BinaryOperator::add:
                    return require_finite_result(lhs + rhs);
                case BinaryOperator::subtract:
                    return require_finite_result(lhs - rhs);
                case BinaryOperator::multiply:
                    return require_finite_result(lhs * rhs);
                case BinaryOperator::divide:
                    if (rhs == 0.0) {
                        throw std::invalid_argument("division by zero");
                    }

                    return require_finite_result(lhs / rhs);
                case BinaryOperator::modulo:
                    if (rhs == 0.0) {
                        throw std::invalid_argument("modulo by zero");
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

                throw std::invalid_argument("unknown binary operator");
            }
        },
        expression.node);
}

}  // namespace console_calc
