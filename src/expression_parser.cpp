#include "console_calc/expression_parser.h"

#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <string_view>

namespace console_calc {

namespace {

enum class TokenKind {
    number,
    plus,
    minus,
    multiply,
    divide,
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

[[nodiscard]] double apply_operator(double lhs, TokenKind op, double rhs) {
    switch (op) {
    case TokenKind::plus:
        return lhs + rhs;
    case TokenKind::minus:
        return lhs - rhs;
    case TokenKind::multiply:
        return lhs * rhs;
    case TokenKind::divide:
        return lhs / rhs;
    case TokenKind::number:
    case TokenKind::end:
        break;
    }

    throw std::invalid_argument("expected binary operator");
}

[[nodiscard]] bool is_binary_operator(TokenKind kind) {
    return kind == TokenKind::plus || kind == TokenKind::minus ||
           kind == TokenKind::multiply || kind == TokenKind::divide;
}

}  // namespace

double ExpressionParser::evaluate(const std::string& expression) const {
    Tokenizer tokenizer(expression);

    const Token first = tokenizer.next();
    if (first.kind != TokenKind::number) {
        throw std::invalid_argument("expression must start with a number");
    }

    double result = first.number_value;

    while (true) {
        const Token op = tokenizer.next();
        if (op.kind == TokenKind::end) {
            return result;
        }

        if (!is_binary_operator(op.kind)) {
            throw std::invalid_argument("expected binary operator");
        }

        const Token rhs = tokenizer.next();
        if (rhs.kind != TokenKind::number) {
            throw std::invalid_argument("expected number after operator");
        }

        result = apply_operator(result, op.kind, rhs.number_value);
    }
}

}  // namespace console_calc
