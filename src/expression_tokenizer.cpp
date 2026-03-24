#include "expression_tokenizer.h"

#include "console_calc/expression_error.h"

#include <charconv>
#include <cctype>
#include <cerrno>
#include <cstdint>
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
    case '~':
        ++position_;
        return {.kind = TokenKind::bitwise_not};
    case '=':
        ++position_;
        return {.kind = TokenKind::equal};
    case '<':
        ++position_;
        if (position_ < input_.size() && input_[position_] == '=') {
            ++position_;
            return {.kind = TokenKind::less_equal};
        }
        return {.kind = TokenKind::less};
    case '>':
        ++position_;
        if (position_ < input_.size() && input_[position_] == '=') {
            ++position_;
            return {.kind = TokenKind::greater_equal};
        }
        return {.kind = TokenKind::greater};
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
    case '{':
        ++position_;
        return {.kind = TokenKind::left_brace};
    case '}':
        ++position_;
        return {.kind = TokenKind::right_brace};
    case ',':
        ++position_;
        return {.kind = TokenKind::comma};
    default:
        break;
    }

    if (std::isdigit(static_cast<unsigned char>(current)) || current == '.') {
        return parse_number();
    }
    if (std::isalpha(static_cast<unsigned char>(current)) || current == '_') {
        return parse_identifier();
    }

    throw ParseError("unexpected character in expression");
}

Token Tokenizer::parse_number() {
    const std::size_t begin_index = position_;
    if (input_[position_] == '0' && position_ + 2 <= input_.size()) {
        const char prefix = input_[position_ + 1];
        if (prefix == 'x' || prefix == 'X' || prefix == 'b' || prefix == 'B') {
            const int base = (prefix == 'x' || prefix == 'X') ? 16 : 2;
            std::size_t digit_begin = position_ + 2;
            std::size_t end_index = digit_begin;
            while (end_index < input_.size()) {
                const char ch = input_[end_index];
                const bool valid_digit = base == 16
                                             ? std::isxdigit(static_cast<unsigned char>(ch)) != 0
                                             : (ch == '0' || ch == '1');
                if (!valid_digit) {
                    break;
                }
                ++end_index;
            }

            if (end_index == digit_begin) {
                throw ParseError("invalid number literal");
            }

            std::int64_t integer_value = 0;
            const auto [ptr, ec] = std::from_chars(
                input_.data() + digit_begin, input_.data() + end_index, integer_value, base);
            if (ec != std::errc{} || ptr != input_.data() + end_index) {
                throw ParseError("invalid number literal");
            }

            position_ = end_index;
            return {
                .kind = TokenKind::number,
                .number_value = integer_value,
            };
        }
    }

    const char* begin = input_.data() + position_;
    char* end = nullptr;
    const double value = std::strtod(begin, &end);

    if (end == begin) {
        throw ParseError("invalid number literal");
    }

    position_ = static_cast<std::size_t>(end - input_.data());
    const std::string_view token_text = input_.substr(begin_index, position_ - begin_index);
    if (token_text.find_first_of(".eE") == std::string_view::npos) {
        std::int64_t integer_value = 0;
        const auto [ptr, ec] = std::from_chars(
            token_text.data(), token_text.data() + token_text.size(), integer_value);
        if (ec == std::errc{} && ptr == token_text.data() + token_text.size()) {
            return {
                .kind = TokenKind::number,
                .number_value = integer_value,
            };
        }
    }

    return {
        .kind = TokenKind::number,
        .number_value = value,
    };
}

Token Tokenizer::parse_identifier() {
    const std::size_t begin = position_;
    ++position_;

    while (position_ < input_.size()) {
        const char current = input_[position_];
        if (!std::isalnum(static_cast<unsigned char>(current)) && current != '_') {
            break;
        }
        ++position_;
    }

    return {
        .kind = TokenKind::identifier,
        .identifier_text = input_.substr(begin, position_ - begin),
    };
}

void Tokenizer::skip_whitespace() {
    while (position_ < input_.size() &&
           std::isspace(static_cast<unsigned char>(input_[position_]))) {
        ++position_;
    }
}

}  // namespace console_calc
