#include "expression_ast_parser.h"

#include "expression_ast_parser_internal.h"

#include "console_calc/expression_error.h"

#include <memory>
#include <utility>

namespace console_calc::detail {

BinaryOperator to_binary_operator(TokenKind kind) {
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

std::unique_ptr<Expression> make_expression(Expression expression) {
    return std::make_unique<Expression>(std::move(expression));
}

bool starts_primary_expression(TokenKind kind) {
    return kind == TokenKind::number || kind == TokenKind::identifier ||
           kind == TokenKind::left_paren || kind == TokenKind::left_brace;
}

bool starts_operand_expression(TokenKind kind) {
    return starts_primary_expression(kind) || kind == TokenKind::minus ||
           kind == TokenKind::bitwise_not;
}

Parser::Parser(std::string_view input)
    : tokenizer_(input), current_(tokenizer_.next()), next_(tokenizer_.next()) {}

Expression Parser::parse() {
    if (!starts_operand_expression(current_.kind)) {
        throw ParseError(
            "expression must start with a number, function, unary operator, '(' or '{'");
    }

    Expression expression = parse_bitwise_or_expression();
    if (current_.kind != TokenKind::end) {
        throw ParseError("expected binary operator");
    }

    return expression;
}

Expression Parser::parse_bitwise_or_expression() {
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

Expression Parser::parse_bitwise_and_expression() {
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

Expression Parser::parse_comparison_expression() {
    Expression expression = parse_additive_expression();

    while (current_.kind == TokenKind::equal || current_.kind == TokenKind::less ||
           current_.kind == TokenKind::less_equal || current_.kind == TokenKind::greater ||
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

Expression Parser::parse_additive_expression() {
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

Expression Parser::parse_multiplicative_expression() {
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

Expression Parser::parse_unary_expression() {
    if (current_.kind == TokenKind::minus || current_.kind == TokenKind::bitwise_not) {
        const UnaryOperator op =
            current_.kind == TokenKind::minus ? UnaryOperator::negate
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

Expression Parser::parse_power_expression() {
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

Expression Parser::parse_postfix_expression() {
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

Expression Parser::parse_primary_expression() {
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

Expression Parser::parse_list_literal() {
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

void Parser::advance() {
    current_ = next_;
    next_ = tokenizer_.next();
}

}  // namespace console_calc::detail

namespace console_calc {

Expression parse_expression(std::string_view input) {
    detail::Parser parser(input);
    return parser.parse();
}

}  // namespace console_calc
