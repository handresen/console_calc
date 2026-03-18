#include "expression_ast_parser.h"

#include "console_calc/expression_error.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
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
    case TokenKind::comma:
    case TokenKind::end:
        break;
    }

    throw ParseError("expected binary operator");
}

[[nodiscard]] std::optional<Function> to_function(std::string_view name) {
    if (name == "sin") {
        return Function::sin;
    }
    if (name == "cos") {
        return Function::cos;
    }
    if (name == "tan") {
        return Function::tan;
    }
    if (name == "sind") {
        return Function::sind;
    }
    if (name == "cosd") {
        return Function::cosd;
    }
    if (name == "tand") {
        return Function::tand;
    }
    if (name == "pow") {
        return Function::pow;
    }
    if (name == "sum") {
        return Function::sum;
    }

    return std::nullopt;
}

[[nodiscard]] std::size_t function_arity(Function function) {
    switch (function) {
    case Function::sin:
    case Function::cos:
    case Function::tan:
    case Function::sind:
    case Function::cosd:
    case Function::tand:
        return 1;
    case Function::pow:
        return 2;
    case Function::sum:
        return 1;
    }

    throw ParseError("unknown function");
}

[[nodiscard]] std::string function_name(Function function) {
    switch (function) {
    case Function::sin:
        return "sin";
    case Function::cos:
        return "cos";
    case Function::tan:
        return "tan";
    case Function::sind:
        return "sind";
    case Function::cosd:
        return "cosd";
    case Function::tand:
        return "tand";
    case Function::pow:
        return "pow";
    case Function::sum:
        return "sum";
    }

    throw ParseError("unknown function");
}

[[nodiscard]] std::unique_ptr<Expression> make_expression(Expression expression) {
    return std::make_unique<Expression>(std::move(expression));
}

[[nodiscard]] bool starts_primary_expression(TokenKind kind) {
    return kind == TokenKind::number || kind == TokenKind::identifier ||
           kind == TokenKind::left_paren || kind == TokenKind::left_brace;
}

[[nodiscard]] bool starts_operand_expression(TokenKind kind) {
    return starts_primary_expression(kind) || kind == TokenKind::minus;
}

class Parser {
public:
    explicit Parser(std::string_view input) : tokenizer_(input), current_(tokenizer_.next()) {}

    [[nodiscard]] Expression parse() {
        if (!starts_operand_expression(current_.kind)) {
            throw ParseError("expression must start with a number, function, '-', '(' or '{'");
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
        Expression expression = parse_additive_expression();

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
        if (current_.kind == TokenKind::minus) {
            advance();

            if (!starts_operand_expression(current_.kind)) {
                throw ParseError("expected number after operator");
            }

            return Expression{
                UnaryExpression{
                    .operand = make_expression(parse_unary_expression()),
                }};
        }

        return parse_power_expression();
    }

    [[nodiscard]] Expression parse_power_expression() {
        Expression expression = parse_primary_expression();

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

    [[nodiscard]] Expression parse_primary_expression() {
        if (current_.kind == TokenKind::identifier) {
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

    [[nodiscard]] Expression parse_function_call() {
        const auto function = to_function(current_.identifier_text);
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
            if (*function == Function::sum) {
                arguments.push_back(make_expression(parse_sum_argument()));
            } else {
                arguments.push_back(make_expression(parse_bitwise_or_expression()));
                while (current_.kind == TokenKind::comma) {
                    advance();
                    if (!starts_operand_expression(current_.kind)) {
                        throw ParseError("expected expression after ','");
                    }
                    arguments.push_back(make_expression(parse_bitwise_or_expression()));
                }
            }
        }

        if (current_.kind != TokenKind::right_paren) {
            throw ParseError("expected ')'");
        }

        const std::size_t expected_arity = function_arity(*function);
        if (arguments.size() != expected_arity) {
            throw ParseError("function '" + function_name(*function) + "' expects " +
                             std::to_string(expected_arity) +
                             (expected_arity == 1 ? " argument" : " arguments"));
        }

        advance();
        return Expression{
            FunctionCall{
                .function = *function,
                .arguments = std::move(arguments),
            }};
    }

    [[nodiscard]] Expression parse_sum_argument() {
        if (current_.kind == TokenKind::left_brace) {
            return parse_list_literal();
        }

        return parse_bitwise_or_expression();
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
