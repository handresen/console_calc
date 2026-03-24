#include "expression_ast_parser.h"

#include "console_calc/builtin_function.h"
#include "console_calc/expression_error.h"
#include "console_calc/special_form.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "expression_tokenizer.h"

namespace console_calc {

namespace {

[[nodiscard]] BinaryOperator to_binary_operator(TokenKind kind) {
    switch (kind) {
    case TokenKind::plus:
        return BinaryOperator::add;
    case TokenKind::minus:
        return BinaryOperator::subtract;
    case TokenKind::bitwise_not:
    case TokenKind::equal:
        return BinaryOperator::equal;
    case TokenKind::less:
        return BinaryOperator::less;
    case TokenKind::less_equal:
        return BinaryOperator::less_equal;
    case TokenKind::greater:
        return BinaryOperator::greater;
    case TokenKind::greater_equal:
        return BinaryOperator::greater_equal;
    case TokenKind::multiply:
        return BinaryOperator::multiply;
    case TokenKind::divide:
        return BinaryOperator::divide;
    case TokenKind::modulo:
        return BinaryOperator::modulo;
    case TokenKind::power:
        return BinaryOperator::power;
    case TokenKind::bitwise_and:
        return BinaryOperator::bitwise_and;
    case TokenKind::bitwise_or:
        return BinaryOperator::bitwise_or;
    case TokenKind::number:
    case TokenKind::identifier:
    case TokenKind::left_paren:
    case TokenKind::right_paren:
    case TokenKind::left_brace:
    case TokenKind::right_brace:
    case TokenKind::left_bracket:
    case TokenKind::right_bracket:
    case TokenKind::comma:
    case TokenKind::end:
        break;
    }

    throw ParseError("expected binary operator");
}

[[nodiscard]] std::unique_ptr<Expression> make_expression(Expression expression) {
    return std::make_unique<Expression>(std::move(expression));
}

[[nodiscard]] bool starts_primary_expression(TokenKind kind) {
    return kind == TokenKind::number || kind == TokenKind::identifier ||
           kind == TokenKind::left_paren || kind == TokenKind::left_brace;
}

[[nodiscard]] bool starts_operand_expression(TokenKind kind) {
    return starts_primary_expression(kind) || kind == TokenKind::minus ||
           kind == TokenKind::bitwise_not;
}

class Parser {
public:
    explicit Parser(std::string_view input)
        : tokenizer_(input), current_(tokenizer_.next()), next_(tokenizer_.next()) {}

    [[nodiscard]] Expression parse() {
        if (!starts_operand_expression(current_.kind)) {
            throw ParseError("expression must start with a number, function, unary operator, '(' or '{'");
        }

        Expression expression = parse_bitwise_or_expression();
        if (current_.kind != TokenKind::end) {
            throw ParseError("expected binary operator");
        }

        return expression;
    }

private:
    [[nodiscard]] Expression parse_bitwise_or_expression() {
        Expression expression = parse_bitwise_and_expression();

        while (current_.kind == TokenKind::bitwise_or) {
            const TokenKind op = current_.kind;
            advance();

            if (!starts_operand_expression(current_.kind)) {
                throw ParseError("expected number after operator");
            }

            expression = Expression{
                BinaryExpression{
                    .op = to_binary_operator(op),
                    .left = make_expression(std::move(expression)),
                    .right = make_expression(parse_bitwise_and_expression()),
                }};
        }

        return expression;
    }

    [[nodiscard]] Expression parse_bitwise_and_expression() {
        Expression expression = parse_comparison_expression();

        while (current_.kind == TokenKind::bitwise_and) {
            const TokenKind op = current_.kind;
            advance();

            if (!starts_operand_expression(current_.kind)) {
                throw ParseError("expected number after operator");
            }

            expression = Expression{
                BinaryExpression{
                    .op = to_binary_operator(op),
                    .left = make_expression(std::move(expression)),
                    .right = make_expression(parse_comparison_expression()),
                }};
        }

        return expression;
    }

    [[nodiscard]] Expression parse_comparison_expression() {
        Expression expression = parse_additive_expression();

        while (current_.kind == TokenKind::equal || current_.kind == TokenKind::less ||
               current_.kind == TokenKind::less_equal ||
               current_.kind == TokenKind::greater ||
               current_.kind == TokenKind::greater_equal) {
            const TokenKind op = current_.kind;
            advance();

            if (!starts_operand_expression(current_.kind)) {
                throw ParseError("expected number after operator");
            }

            expression = Expression{
                BinaryExpression{
                    .op = to_binary_operator(op),
                    .left = make_expression(std::move(expression)),
                    .right = make_expression(parse_additive_expression()),
                }};
        }

        return expression;
    }

    [[nodiscard]] Expression parse_additive_expression() {
        Expression expression = parse_multiplicative_expression();

        while (current_.kind == TokenKind::plus || current_.kind == TokenKind::minus) {
            const TokenKind op = current_.kind;
            advance();

            if (!starts_operand_expression(current_.kind)) {
                throw ParseError("expected number after operator");
            }

            expression = Expression{
                BinaryExpression{
                    .op = to_binary_operator(op),
                    .left = make_expression(std::move(expression)),
                    .right = make_expression(parse_multiplicative_expression()),
                }};
        }

        return expression;
    }

    [[nodiscard]] Expression parse_multiplicative_expression() {
        Expression expression = parse_unary_expression();

        while (current_.kind == TokenKind::multiply || current_.kind == TokenKind::divide ||
               current_.kind == TokenKind::modulo) {
            const TokenKind op = current_.kind;
            advance();

            if (!starts_operand_expression(current_.kind)) {
                throw ParseError("expected number after operator");
            }

            expression = Expression{
                BinaryExpression{
                    .op = to_binary_operator(op),
                    .left = make_expression(std::move(expression)),
                    .right = make_expression(parse_unary_expression()),
                }};
        }

        return expression;
    }

    [[nodiscard]] Expression parse_unary_expression() {
        if (current_.kind == TokenKind::minus || current_.kind == TokenKind::bitwise_not) {
            const UnaryOperator op = current_.kind == TokenKind::minus
                                         ? UnaryOperator::negate
                                         : UnaryOperator::bitwise_not;
            advance();

            if (!starts_operand_expression(current_.kind)) {
                throw ParseError("expected number after operator");
            }

            return Expression{
                UnaryExpression{
                    .op = op,
                    .operand = make_expression(parse_unary_expression()),
                }};
        }

        return parse_power_expression();
    }

    [[nodiscard]] Expression parse_power_expression() {
        Expression expression = parse_postfix_expression();

        if (current_.kind == TokenKind::power) {
            advance();

            if (!starts_operand_expression(current_.kind)) {
                throw ParseError("expected number after operator");
            }

            expression = Expression{
                BinaryExpression{
                    .op = BinaryOperator::power,
                    .left = make_expression(std::move(expression)),
                    .right = make_expression(parse_unary_expression()),
                }};
        }

        return expression;
    }

    [[nodiscard]] Expression parse_postfix_expression() {
        Expression expression = parse_primary_expression();

        while (current_.kind == TokenKind::left_bracket) {
            advance();
            Expression index_expression = parse_bitwise_or_expression();
            if (current_.kind != TokenKind::right_bracket) {
                throw ParseError("expected ']'");
            }
            advance();
            expression = Expression{
                IndexExpression{
                    .collection = make_expression(std::move(expression)),
                    .index = make_expression(std::move(index_expression)),
                }};
        }

        return expression;
    }

    [[nodiscard]] Expression parse_primary_expression() {
        if (current_.kind == TokenKind::identifier) {
            if (allow_placeholder_expression_ && current_.identifier_text == "_") {
                advance();
                return Expression{PlaceholderExpression{}};
            }
            return parse_function_call();
        }

        if (current_.kind == TokenKind::left_brace) {
            return parse_list_literal();
        }

        if (current_.kind == TokenKind::left_paren) {
            advance();
            Expression expression = parse_bitwise_or_expression();
            if (current_.kind != TokenKind::right_paren) {
                throw ParseError("expected ')'");
            }

            advance();
            return expression;
        }

        if (current_.kind != TokenKind::number) {
            throw ParseError("expected number after operator");
        }

        Expression expression{NumberLiteral{.value = current_.number_value}};
        advance();
        return expression;
    }

    [[nodiscard]] Expression parse_special_form_call(Function form) {
        switch (form) {
        case Function::map:
        case Function::map_at:
            return parse_map_call();
        case Function::list_where:
            return parse_list_where_call();
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

    [[nodiscard]] Expression parse_function_call() {
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
                             "' expects " +
                             std::string(builtin_function_signature(*function)));
        }

        advance();
        return Expression{
            FunctionCall{
                .function = *function,
                .arguments = std::move(arguments),
            }};
    }

    [[nodiscard]] Expression parse_map_call() {
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

    [[nodiscard]] Expression parse_list_where_call() {
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

    [[nodiscard]] Expression parse_guard_call() {
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

    [[nodiscard]] Expression parse_reduce_call() {
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

    [[nodiscard]] Expression parse_timed_loop_call() {
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

    [[nodiscard]] Expression parse_fill_call() {
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

    [[nodiscard]] Expression parse_list_literal() {
        advance();

        std::vector<std::unique_ptr<Expression>> elements;
        if (current_.kind != TokenKind::right_brace) {
            if (!starts_operand_expression(current_.kind)) {
                throw ParseError("expected expression after '{'");
            }

            elements.push_back(make_expression(parse_bitwise_or_expression()));
            while (current_.kind == TokenKind::comma) {
                advance();
                if (!starts_operand_expression(current_.kind)) {
                    throw ParseError("expected expression after ','");
                }
                elements.push_back(make_expression(parse_bitwise_or_expression()));
            }
        }

        if (current_.kind != TokenKind::right_brace) {
            throw ParseError("expected '}'");
        }

        advance();
        return Expression{ListLiteral{.elements = std::move(elements)}};
    }

    void advance() {
        current_ = next_;
        next_ = tokenizer_.next();
    }

    Tokenizer tokenizer_;
    Token current_;
    Token next_;
    bool allow_placeholder_expression_ = false;
};

}  // namespace

Expression parse_expression(std::string_view input) {
    Parser parser(input);
    return parser.parse();
}

}  // namespace console_calc
