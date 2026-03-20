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
    rand,
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
    guard,
    reduce,
    timed_loop,
    fill,
    map,
    map_at,
    range,
    geom,
    repeat,
    linspace,
    powers,
    pos,
    lat,
    lon,
    to_poslist,
    to_list,
    dist,
    bearing,
    br_to_pos,
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
    std::unique_ptr<Expression> mapped_expression;
    std::unique_ptr<Expression> start_argument;
    std::unique_ptr<Expression> step_argument;
    std::unique_ptr<Expression> count_argument;
    bool preserve_unmapped = false;
};

struct GuardCall {
    std::unique_ptr<Expression> guarded_expression;
    std::unique_ptr<Expression> fallback_expression;
};

struct ReduceCall {
    std::unique_ptr<Expression> list_argument;
    BinaryOperator reduction_operator;
};

struct TimedLoopCall {
    std::unique_ptr<Expression> loop_expression;
    std::unique_ptr<Expression> iteration_count;
};

struct FillCall {
    std::unique_ptr<Expression> fill_expression;
    std::unique_ptr<Expression> iteration_count;
};

struct Expression {
    std::variant<NumberLiteral, PlaceholderExpression, UnaryExpression, BinaryExpression,
                 ListLiteral, FunctionCall, MapCall, GuardCall, ReduceCall, TimedLoopCall,
                 FillCall>
        node;
};

}  // namespace console_calc
