#pragma once

#include <memory>
#include <vector>
#include <variant>

#include "console_calc/value.h"

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
    ScalarValue value = std::int64_t{0};
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
    sum,
    len,
    product,
    avg,
    min,
    max,
    first,
    drop,
    map,
    range,
    geom,
    repeat,
    linspace,
};

struct BinaryExpression {
    BinaryOperator op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
};

struct ListLiteral {
    std::vector<std::unique_ptr<Expression>> elements;
};

struct FunctionCall {
    Function function;
    std::vector<std::unique_ptr<Expression>> arguments;
};

struct MapCall {
    std::unique_ptr<Expression> list_argument;
    Function mapped_function;
};

struct Expression {
    std::variant<NumberLiteral, UnaryExpression, BinaryExpression, ListLiteral, FunctionCall,
                 MapCall>
        node;
};

}  // namespace console_calc
