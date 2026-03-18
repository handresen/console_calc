#include "console_calc/expression_parser.h"

#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace console_calc {

namespace {

enum class TokenKind {
    number,
    plus,
    minus,
    multiply,
    divide,
    modulo,
    power,
    bitwise_and,
    bitwise_or,
    left_paren,
    right_paren,
    end,
};

struct Token {
    TokenKind kind;
    double number_value = 0.0;
};

class Tokenizer {
public:
    explicit Tokenizer(std::string_view input) : input_(input) {}

    [[nodiscard]] Token next() {
        skip_whitespace();

        if (position_ >= input_.size()) {
            return {.kind = TokenKind::end};
        }

        const char current = input_[position_];
        switch (current) {
        case '+':
            ++position_;
            return {.kind = TokenKind::plus};
        case '-':
            ++position_;
            return {.kind = TokenKind::minus};
        case '*':
            ++position_;
            return {.kind = TokenKind::multiply};
        case '/':
            ++position_;
            return {.kind = TokenKind::divide};
        case '%':
            ++position_;
            return {.kind = TokenKind::modulo};
        case '^':
            ++position_;
            return {.kind = TokenKind::power};
        case '&':
            ++position_;
            return {.kind = TokenKind::bitwise_and};
        case '|':
            ++position_;
            return {.kind = TokenKind::bitwise_or};
        case '(':
            ++position_;
            return {.kind = TokenKind::left_paren};
        case ')':
            ++position_;
            return {.kind = TokenKind::right_paren};
        default:
            break;
        }

        if (std::isdigit(static_cast<unsigned char>(current)) || current == '.') {
            return parse_number();
        }

        throw std::invalid_argument("unexpected character in expression");
    }

private:
    [[nodiscard]] Token parse_number() {
        const char* begin = input_.data() + position_;
        char* end = nullptr;
        const double value = std::strtod(begin, &end);

        if (end == begin) {
            throw std::invalid_argument("invalid number literal");
        }

        position_ = static_cast<std::size_t>(end - input_.data());
        return {
            .kind = TokenKind::number,
            .number_value = value,
        };
    }

    void skip_whitespace() {
        while (position_ < input_.size() &&
               std::isspace(static_cast<unsigned char>(input_[position_]))) {
            ++position_;
        }
    }

    std::string_view input_;
    std::size_t position_ = 0;
};

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

[[nodiscard]] std::int64_t require_integer_operand(double value) {
    double integral_part = 0.0;
    if (std::modf(value, &integral_part) != 0.0) {
        throw std::invalid_argument("bitwise operators require integer operands");
    }

    return static_cast<std::int64_t>(integral_part);
}

[[nodiscard]] double evaluate_expression(const Expression& expression) {
    return std::visit(
        [](const auto& node) -> double {
            using Node = std::decay_t<decltype(node)>;

            if constexpr (std::is_same_v<Node, NumberLiteral>) {
                return node.value;
            } else {
                const double lhs = evaluate_expression(*node.left);
                const double rhs = evaluate_expression(*node.right);

                switch (node.op) {
                case BinaryOperator::add:
                    return lhs + rhs;
                case BinaryOperator::subtract:
                    return lhs - rhs;
                case BinaryOperator::multiply:
                    return lhs * rhs;
                case BinaryOperator::divide:
                    return lhs / rhs;
                case BinaryOperator::modulo:
                    return std::fmod(lhs, rhs);
                case BinaryOperator::power:
                    return std::pow(lhs, rhs);
                case BinaryOperator::bitwise_and:
                    return static_cast<double>(require_integer_operand(lhs) &
                                               require_integer_operand(rhs));
                case BinaryOperator::bitwise_or:
                    return static_cast<double>(require_integer_operand(lhs) |
                                               require_integer_operand(rhs));
                }

                throw std::invalid_argument("unknown binary operator");
            }
        },
        expression.node);
}

}  // namespace

Expression ExpressionParser::parse(const std::string& expression) const {
    Parser parser(expression);
    return parser.parse();
}

double ExpressionParser::evaluate(const std::string& expression) const {
    return evaluate_expression(parse(expression));
}

}  // namespace console_calc
