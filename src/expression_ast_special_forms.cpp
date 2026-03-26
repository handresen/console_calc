#include "expression_ast_parser_internal.h"

#include "console_calc/builtin_function.h"
#include "console_calc/expression_error.h"
#include "console_calc/special_form.h"

#include <string>
#include <utility>
#include <vector>

namespace console_calc::detail {

Expression Parser::parse_special_form_call(Function form) {
    switch (form) {
    case Function::map:
    case Function::map_at:
        return parse_map_call();
    case Function::list_where:
        return parse_list_where_call();
    case Function::sort_by:
        return parse_sort_by_call();
    case Function::guard:
        return parse_guard_call();
    case Function::reduce:
        return parse_reduce_call();
    case Function::timed_loop:
        return parse_timed_loop_call();
    case Function::fill:
        return parse_fill_call();
    default:
        break;
    }

    throw ParseError("unknown special form");
}

Expression Parser::parse_function_call() {
    if (const auto special_form = parse_special_form_function(current_.identifier_text);
        special_form.has_value()) {
        return parse_special_form_call(*special_form);
    }

    const auto function = parse_builtin_function(current_.identifier_text);
    if (!function.has_value()) {
        throw ParseError("unknown function");
    }

    advance();
    if (current_.kind != TokenKind::left_paren) {
        throw ParseError("expected '(' after function name");
    }

    advance();
    std::vector<std::unique_ptr<Expression>> arguments;
    if (current_.kind != TokenKind::right_paren) {
        arguments.push_back(make_expression(parse_bitwise_or_expression()));
        while (current_.kind == TokenKind::comma) {
            advance();
            if (!starts_operand_expression(current_.kind)) {
                throw ParseError("expected expression after ','");
            }
            arguments.push_back(make_expression(parse_bitwise_or_expression()));
        }
    }

    if (current_.kind != TokenKind::right_paren) {
        throw ParseError("expected ')'");
    }

    if (!builtin_function_accepts_arity(*function, arguments.size())) {
        throw ParseError("function '" + std::string(builtin_function_name(*function)) +
                         "' expects " + std::string(builtin_function_signature(*function)));
    }

    advance();
    return Expression{
        FunctionCall{
            .function = *function,
            .arguments = std::move(arguments),
        }};
}

Expression Parser::parse_map_call() {
    const auto map_function = parse_special_form_function(current_.identifier_text).value();
    const auto map_name = std::string(builtin_function_name(map_function));
    const auto map_signature = std::string(special_form_signature(map_function));
    advance();
    if (current_.kind != TokenKind::left_paren) {
        throw ParseError("expected '(' after function name");
    }

    advance();
    if (!starts_operand_expression(current_.kind)) {
        throw ParseError("function '" + map_name + "' expects " + map_signature);
    }

    auto list_argument = make_expression(parse_bitwise_or_expression());
    if (current_.kind != TokenKind::comma) {
        throw ParseError("function '" + map_name + "' expects " + map_signature);
    }

    advance();
    if (!starts_operand_expression(current_.kind)) {
        throw ParseError("function '" + map_name + "' expects " + map_signature);
    }

    const bool previous_allow_placeholder_expression = allow_placeholder_expression_;
    allow_placeholder_expression_ = true;
    auto mapped_expression = make_expression(parse_bitwise_or_expression());
    allow_placeholder_expression_ = previous_allow_placeholder_expression;

    std::unique_ptr<Expression> start_argument;
    std::unique_ptr<Expression> step_argument;
    std::unique_ptr<Expression> count_argument;
    if (current_.kind == TokenKind::comma) {
        advance();
        if (!starts_operand_expression(current_.kind)) {
            throw ParseError("function '" + map_name + "' expects " + map_signature);
        }
        start_argument = make_expression(parse_bitwise_or_expression());
    }
    if (current_.kind == TokenKind::comma) {
        advance();
        if (!starts_operand_expression(current_.kind)) {
            throw ParseError("function '" + map_name + "' expects " + map_signature);
        }
        step_argument = make_expression(parse_bitwise_or_expression());
    }
    if (current_.kind == TokenKind::comma) {
        advance();
        if (!starts_operand_expression(current_.kind)) {
            throw ParseError("function '" + map_name + "' expects " + map_signature);
        }
        count_argument = make_expression(parse_bitwise_or_expression());
    }

    if (current_.kind != TokenKind::right_paren) {
        throw ParseError("function '" + map_name + "' expects " + map_signature);
    }

    advance();
    return Expression{
        MapCall{
            .list_argument = std::move(list_argument),
            .mapped_expression = std::move(mapped_expression),
            .start_argument = std::move(start_argument),
            .step_argument = std::move(step_argument),
            .count_argument = std::move(count_argument),
            .preserve_unmapped = (map_function == Function::map_at),
        }};
}

Expression Parser::parse_list_where_call() {
    const auto filter_signature = std::string(special_form_signature(Function::list_where));
    advance();
    if (current_.kind != TokenKind::left_paren) {
        throw ParseError("expected '(' after function name");
    }

    advance();
    if (!starts_operand_expression(current_.kind)) {
        throw ParseError("function 'list_where' expects " + filter_signature);
    }

    auto list_argument = make_expression(parse_bitwise_or_expression());
    if (current_.kind != TokenKind::comma) {
        throw ParseError("function 'list_where' expects " + filter_signature);
    }

    advance();
    if (!starts_operand_expression(current_.kind)) {
        throw ParseError("function 'list_where' expects " + filter_signature);
    }

    const bool previous_allow_placeholder_expression = allow_placeholder_expression_;
    allow_placeholder_expression_ = true;
    auto predicate_expression = make_expression(parse_bitwise_or_expression());
    allow_placeholder_expression_ = previous_allow_placeholder_expression;

    if (current_.kind != TokenKind::right_paren) {
        throw ParseError("function 'list_where' expects " + filter_signature);
    }

    advance();
    return Expression{
        ListWhereCall{
            .list_argument = std::move(list_argument),
            .predicate_expression = std::move(predicate_expression),
        }};
}

Expression Parser::parse_sort_by_call() {
    const auto sort_signature = std::string(special_form_signature(Function::sort_by));
    advance();
    if (current_.kind != TokenKind::left_paren) {
        throw ParseError("expected '(' after function name");
    }

    advance();
    if (!starts_operand_expression(current_.kind)) {
        throw ParseError("function 'sort_by' expects " + sort_signature);
    }

    auto list_argument = make_expression(parse_bitwise_or_expression());
    if (current_.kind != TokenKind::comma) {
        throw ParseError("function 'sort_by' expects " + sort_signature);
    }

    advance();
    if (!starts_operand_expression(current_.kind)) {
        throw ParseError("function 'sort_by' expects " + sort_signature);
    }

    const bool previous_allow_placeholder_expression = allow_placeholder_expression_;
    allow_placeholder_expression_ = true;
    auto key_expression = make_expression(parse_bitwise_or_expression());
    allow_placeholder_expression_ = previous_allow_placeholder_expression;

    if (current_.kind != TokenKind::right_paren) {
        throw ParseError("function 'sort_by' expects " + sort_signature);
    }

    advance();
    return Expression{
        SortByCall{
            .list_argument = std::move(list_argument),
            .key_expression = std::move(key_expression),
        }};
}

Expression Parser::parse_guard_call() {
    const auto guard_signature = std::string(special_form_signature(Function::guard));
    advance();
    if (current_.kind != TokenKind::left_paren) {
        throw ParseError("expected '(' after function name");
    }

    advance();
    if (!starts_operand_expression(current_.kind)) {
        throw ParseError("function 'guard' expects " + guard_signature);
    }

    auto guarded_expression = make_expression(parse_bitwise_or_expression());
    if (current_.kind != TokenKind::comma) {
        throw ParseError("function 'guard' expects " + guard_signature);
    }

    advance();
    if (!starts_operand_expression(current_.kind)) {
        throw ParseError("function 'guard' expects " + guard_signature);
    }

    auto fallback_expression = make_expression(parse_bitwise_or_expression());
    if (current_.kind != TokenKind::right_paren) {
        throw ParseError("function 'guard' expects " + guard_signature);
    }

    advance();
    return Expression{
        GuardCall{
            .guarded_expression = std::move(guarded_expression),
            .fallback_expression = std::move(fallback_expression),
        }};
}

Expression Parser::parse_reduce_call() {
    advance();
    if (current_.kind != TokenKind::left_paren) {
        throw ParseError("expected '(' after function name");
    }

    advance();
    if (!starts_operand_expression(current_.kind)) {
        throw ParseError("expected expression after '('");
    }

    auto list_argument = make_expression(parse_bitwise_or_expression());
    if (current_.kind != TokenKind::comma) {
        throw ParseError("expected ',' after first argument");
    }

    advance();
    if (current_.kind != TokenKind::plus && current_.kind != TokenKind::minus &&
        current_.kind != TokenKind::multiply && current_.kind != TokenKind::divide &&
        current_.kind != TokenKind::modulo && current_.kind != TokenKind::power &&
        current_.kind != TokenKind::bitwise_and &&
        current_.kind != TokenKind::bitwise_or) {
        throw ParseError("expected binary operator after ','");
    }

    const BinaryOperator reduction_operator = to_binary_operator(current_.kind);
    advance();
    if (current_.kind != TokenKind::right_paren) {
        throw ParseError("expected ')'");
    }

    advance();
    return Expression{
        ReduceCall{
            .list_argument = std::move(list_argument),
            .reduction_operator = reduction_operator,
        }};
}

Expression Parser::parse_timed_loop_call() {
    advance();
    if (current_.kind != TokenKind::left_paren) {
        throw ParseError("expected '(' after function name");
    }

    advance();
    if (!starts_operand_expression(current_.kind)) {
        throw ParseError("function 'timed_loop' expects " +
                         std::string(special_form_signature(Function::timed_loop)));
    }

    auto loop_expression = make_expression(parse_bitwise_or_expression());

    if (current_.kind != TokenKind::comma) {
        throw ParseError("function 'timed_loop' expects " +
                         std::string(special_form_signature(Function::timed_loop)));
    }

    advance();
    if (!starts_operand_expression(current_.kind)) {
        throw ParseError("expected expression after ','");
    }

    auto iteration_count = make_expression(parse_bitwise_or_expression());

    if (current_.kind != TokenKind::right_paren) {
        throw ParseError("expected ')'");
    }

    advance();
    return Expression{
        TimedLoopCall{
            .loop_expression = std::move(loop_expression),
            .iteration_count = std::move(iteration_count),
        }};
}

Expression Parser::parse_fill_call() {
    const auto fill_signature = std::string(special_form_signature(Function::fill));
    advance();
    if (current_.kind != TokenKind::left_paren) {
        throw ParseError("expected '(' after function name");
    }

    advance();
    if (!starts_operand_expression(current_.kind)) {
        throw ParseError("function 'fill' expects " + fill_signature);
    }

    auto fill_expression = make_expression(parse_bitwise_or_expression());
    if (current_.kind != TokenKind::comma) {
        throw ParseError("function 'fill' expects " + fill_signature);
    }

    advance();
    if (!starts_operand_expression(current_.kind)) {
        throw ParseError("expected expression after ','");
    }

    auto iteration_count = make_expression(parse_bitwise_or_expression());
    if (current_.kind != TokenKind::right_paren) {
        throw ParseError("expected ')'");
    }

    advance();
    return Expression{
        FillCall{
            .fill_expression = std::move(fill_expression),
            .iteration_count = std::move(iteration_count),
        }};
}

}  // namespace console_calc::detail
