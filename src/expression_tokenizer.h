#pragma once

#include <cstddef>
#include <string_view>

namespace console_calc {

enum class TokenKind {
    number,
    identifier,
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
    left_brace,
    right_brace,
    comma,
    end,
};

struct Token {
    TokenKind kind;
    double number_value = 0.0;
    std::string_view identifier_text;
};

class Tokenizer {
public:
    explicit Tokenizer(std::string_view input);

    [[nodiscard]] Token next();

private:
    [[nodiscard]] Token parse_number();
    [[nodiscard]] Token parse_identifier();
    void skip_whitespace();

    std::string_view input_;
    std::size_t position_ = 0;
};

}  // namespace console_calc
