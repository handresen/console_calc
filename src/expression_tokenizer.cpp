#include "expression_tokenizer.h"

#include "console_calc/expression_error.h"

#include <cctype>
#include <cstdlib>

namespace console_calc {

Tokenizer::Tokenizer(std::string_view input) : input_(input) {}

Token Tokenizer::next() {
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

    throw ParseError("unexpected character in expression");
}

Token Tokenizer::parse_number() {
    const char* begin = input_.data() + position_;
    char* end = nullptr;
    const double value = std::strtod(begin, &end);

    if (end == begin) {
        throw ParseError("invalid number literal");
    }

    position_ = static_cast<std::size_t>(end - input_.data());
    return {
        .kind = TokenKind::number,
        .number_value = value,
    };
}

void Tokenizer::skip_whitespace() {
    while (position_ < input_.size() &&
           std::isspace(static_cast<unsigned char>(input_[position_]))) {
        ++position_;
    }
}

}  // namespace console_calc
