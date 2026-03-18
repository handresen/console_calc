#include "expression_ast_parser.h"

#include <memory>
#include <stdexcept>
#include <utility>

#include "expression_tokenizer.h"

namespace console_calc {

namespace {

[[nodiscard]] BinaryOperator to_binary_operator(TokenKind kind) {
    switch (kind) {
    case TokenKind::plus:
        return BinaryOperator::add;
    case TokenKind::minus:
        return BinaryOperator::subtract;
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
    case TokenKind::left_paren:
    case TokenKind::right_paren:
    case TokenKind::number:
    case TokenKind::end:
        break;
    }

    throw std::invalid_argument("expected binary operator");
}

[[nodiscard]] std::unique_ptr<Expression> make_expression(Expression expression) {
    return std::make_unique<Expression>(std::move(expression));
}

[[nodiscard]] bool starts_primary_expression(TokenKind kind) {
    return kind == TokenKind::number || kind == TokenKind::left_paren;
}

class Parser {
public:
    explicit Parser(std::string_view input) : tokenizer_(input), current_(tokenizer_.next()) {}

    [[nodiscard]] Expression parse() {
        if (!starts_primary_expression(current_.kind)) {
            throw std::invalid_argument("expression must start with a number or '('");
        }

        Expression expression = parse_bitwise_or_expression();
        if (current_.kind != TokenKind::end) {
            throw std::invalid_argument("expected binary operator");
        }

        return expression;
    }

private:
    [[nodiscard]] Expression parse_bitwise_or_expression() {
        Expression expression = parse_bitwise_and_expression();

        while (current_.kind == TokenKind::bitwise_or) {
            const TokenKind op = current_.kind;
            advance();

            if (!starts_primary_expression(current_.kind)) {
                throw std::invalid_argument("expected number after operator");
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
        Expression expression = parse_additive_expression();

        while (current_.kind == TokenKind::bitwise_and) {
            const TokenKind op = current_.kind;
            advance();

            if (!starts_primary_expression(current_.kind)) {
                throw std::invalid_argument("expected number after operator");
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

            if (!starts_primary_expression(current_.kind)) {
                throw std::invalid_argument("expected number after operator");
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
        Expression expression = parse_power_expression();

        while (current_.kind == TokenKind::multiply || current_.kind == TokenKind::divide ||
               current_.kind == TokenKind::modulo) {
            const TokenKind op = current_.kind;
            advance();

            if (!starts_primary_expression(current_.kind)) {
                throw std::invalid_argument("expected number after operator");
            }

            expression = Expression{
                BinaryExpression{
                    .op = to_binary_operator(op),
                    .left = make_expression(std::move(expression)),
                    .right = make_expression(parse_power_expression()),
                }};
        }

        return expression;
    }

    [[nodiscard]] Expression parse_power_expression() {
        Expression expression = parse_primary_expression();

        if (current_.kind == TokenKind::power) {
            advance();

            if (!starts_primary_expression(current_.kind)) {
                throw std::invalid_argument("expected number after operator");
            }

            expression = Expression{
                BinaryExpression{
                    .op = BinaryOperator::power,
                    .left = make_expression(std::move(expression)),
                    .right = make_expression(parse_power_expression()),
                }};
        }

        return expression;
    }

    [[nodiscard]] Expression parse_primary_expression() {
        if (current_.kind == TokenKind::left_paren) {
            advance();
            Expression expression = parse_bitwise_or_expression();
            if (current_.kind != TokenKind::right_paren) {
                throw std::invalid_argument("expected ')'");
            }

            advance();
            return expression;
        }

        if (current_.kind != TokenKind::number) {
            throw std::invalid_argument("expected number after operator");
        }

        Expression expression{NumberLiteral{.value = current_.number_value}};
        advance();
        return expression;
    }

    void advance() {
        current_ = tokenizer_.next();
    }

    Tokenizer tokenizer_;
    Token current_;
};

}  // namespace

Expression parse_expression(std::string_view input) {
    Parser parser(input);
    return parser.parse();
}

}  // namespace console_calc
