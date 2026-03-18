#pragma once

#include <memory>
#include <variant>

namespace console_calc {

enum class BinaryOperator {
    add,
    subtract,
    multiply,
    divide,
    modulo,
    power,
    bitwise_and,
    bitwise_or,
};

struct Expression;

struct NumberLiteral {
    double value = 0.0;
};

struct BinaryExpression {
    BinaryOperator op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
};

struct Expression {
    std::variant<NumberLiteral, BinaryExpression> node;
};

}  // namespace console_calc
