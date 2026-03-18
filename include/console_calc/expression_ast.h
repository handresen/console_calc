#pragma once

#include <memory>
#include <vector>
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

struct UnaryExpression {
    std::unique_ptr<Expression> operand;
};

enum class Function {
    sin,
    cos,
    tan,
    sind,
    cosd,
    tand,
    pow,
};

struct BinaryExpression {
    BinaryOperator op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
};

struct FunctionCall {
    Function function;
    std::vector<std::unique_ptr<Expression>> arguments;
};

struct Expression {
    std::variant<NumberLiteral, UnaryExpression, BinaryExpression, FunctionCall> node;
};

}  // namespace console_calc
