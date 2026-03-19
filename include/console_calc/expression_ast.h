#pragma once

#include <memory>
#include <optional>
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

struct PlaceholderExpression {};

struct UnaryExpression {
    std::unique_ptr<Expression> operand;
};

enum class Function {
    abs,
    sin,
    cos,
    tan,
    sind,
    cosd,
    tand,
    sqrt,
    pow,
    sum,
    len,
    product,
    avg,
    min,
    max,
    first,
    drop,
    list_add,
    list_sub,
    list_div,
    list_mul,
    reduce,
    map,
    range,
    geom,
    repeat,
    linspace,
    powers,
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
    std::optional<Function> mapped_function;
    std::unique_ptr<Expression> mapped_expression;
};

struct ReduceCall {
    std::unique_ptr<Expression> list_argument;
    BinaryOperator reduction_operator;
};

struct Expression {
    std::variant<NumberLiteral, PlaceholderExpression, UnaryExpression, BinaryExpression,
                 ListLiteral, FunctionCall, MapCall, ReduceCall>
        node;
};

}  // namespace console_calc
