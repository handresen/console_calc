#pragma once

#include <memory>
#include <string_view>

#include "console_calc/expression_ast.h"
#include "expression_tokenizer.h"

namespace console_calc::detail {

[[nodiscard]] BinaryOperator to_binary_operator(TokenKind kind);
[[nodiscard]] std::unique_ptr<Expression> make_expression(Expression expression);
[[nodiscard]] bool starts_primary_expression(TokenKind kind);
[[nodiscard]] bool starts_operand_expression(TokenKind kind);

class Parser {
public:
    explicit Parser(std::string_view input);

    [[nodiscard]] Expression parse();

private:
    [[nodiscard]] Expression parse_bitwise_or_expression();
    [[nodiscard]] Expression parse_bitwise_and_expression();
    [[nodiscard]] Expression parse_comparison_expression();
    [[nodiscard]] Expression parse_additive_expression();
    [[nodiscard]] Expression parse_multiplicative_expression();
    [[nodiscard]] Expression parse_unary_expression();
    [[nodiscard]] Expression parse_power_expression();
    [[nodiscard]] Expression parse_postfix_expression();
    [[nodiscard]] Expression parse_primary_expression();
    [[nodiscard]] Expression parse_special_form_call(Function form);
    [[nodiscard]] Expression parse_function_call();
    [[nodiscard]] Expression parse_map_call();
    [[nodiscard]] Expression parse_list_where_call();
    [[nodiscard]] Expression parse_sort_by_call();
    [[nodiscard]] Expression parse_guard_call();
    [[nodiscard]] Expression parse_reduce_call();
    [[nodiscard]] Expression parse_timed_loop_call();
    [[nodiscard]] Expression parse_fill_call();
    [[nodiscard]] Expression parse_list_literal();

    void advance();

    Tokenizer tokenizer_;
    Token current_;
    Token next_;
    bool allow_placeholder_expression_ = false;
};

}  // namespace console_calc::detail
