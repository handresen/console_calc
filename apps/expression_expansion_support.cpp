#include "expression_expansion_support.h"

#include "console_calc/builtin_function.h"
#include "console_calc/special_form.h"

#include <cctype>
#include <span>
#include <stdexcept>

namespace console_calc::detail {

bool is_identifier_start(char ch) {
    return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

bool is_identifier_char(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

bool is_identifier_path_char(char ch) {
    return is_identifier_char(ch) || ch == '.';
}

std::size_t skip_whitespace(std::string_view expression, std::size_t index) {
    while (index < expression.size() &&
           std::isspace(static_cast<unsigned char>(expression[index]))) {
        ++index;
    }

    return index;
}

bool is_blank_text(std::string_view text) {
    return skip_whitespace(text, 0) == text.size();
}

bool is_followed_by_call(std::string_view expression, std::size_t index) {
    index = skip_whitespace(expression, index);
    return index < expression.size() && expression[index] == '(';
}

namespace {

[[nodiscard]] bool is_radix_digit(char ch, int base) {
    return base == 16 ? std::isxdigit(static_cast<unsigned char>(ch)) != 0
                      : (ch == '0' || ch == '1');
}

[[nodiscard]] std::string trim_copy(std::string_view text) {
    std::size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) {
        ++begin;
    }

    std::size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }

    return std::string(text.substr(begin, end - begin));
}

}  // namespace

std::size_t consume_radix_literal(std::string_view expression, std::size_t index) {
    if (index + 2 > expression.size() || expression[index] != '0') {
        return index;
    }

    const char prefix = expression[index + 1];
    if (prefix != 'x' && prefix != 'X' && prefix != 'b' && prefix != 'B') {
        return index;
    }

    const int base = (prefix == 'x' || prefix == 'X') ? 16 : 2;
    std::size_t end = index + 2;
    while (end < expression.size() && is_radix_digit(expression[end], base)) {
        ++end;
    }

    return end > index + 2 ? end : index;
}

bool is_inside_placeholder_expression(const std::vector<ExpansionFrame>& frames) {
    for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
        if (it->kind == ExpansionFrameKind::call &&
            (it->identifier == "map" || it->identifier == "map_at" ||
             it->identifier == "list_where" || it->identifier == "sort_by") &&
            it->argument_index == 1) {
            return true;
        }
    }

    return false;
}

bool is_builtin_or_special_call(std::string_view identifier, bool followed_by_call) {
    return followed_by_call &&
           (is_builtin_function_name(identifier) || is_special_form_name(identifier));
}

std::size_t call_open_paren_index(std::string_view expression, std::size_t index) {
    const std::size_t open_paren = skip_whitespace(expression, index);
    return open_paren < expression.size() && expression[open_paren] == '('
               ? open_paren
               : std::string_view::npos;
}

std::size_t find_call_close_paren(std::string_view expression,
                                  std::size_t open_paren_index) {
    int paren_depth = 0;
    int brace_depth = 0;
    for (std::size_t index = open_paren_index; index < expression.size(); ++index) {
        const std::size_t radix_end = consume_radix_literal(expression, index);
        if (radix_end != index) {
            index = radix_end - 1;
            continue;
        }

        switch (expression[index]) {
        case '(':
            ++paren_depth;
            break;
        case ')':
            --paren_depth;
            if (paren_depth == 0) {
                return index;
            }
            break;
        case '{':
            ++brace_depth;
            break;
        case '}':
            --brace_depth;
            break;
        default:
            break;
        }

        if (paren_depth < 0 || brace_depth < 0) {
            break;
        }
    }

    throw std::invalid_argument("expected ')'");
}

std::vector<std::string> extract_call_arguments(std::string_view expression,
                                                std::size_t open_paren_index,
                                                std::size_t close_paren_index) {
    std::size_t argument_begin = open_paren_index + 1;
    int paren_depth = 0;
    int brace_depth = 0;
    std::vector<std::string> arguments;
    std::size_t item_begin = argument_begin;
    for (std::size_t index = argument_begin; index < close_paren_index; ++index) {
        const std::size_t radix_end = consume_radix_literal(expression, index);
        if (radix_end != index) {
            index = radix_end - 1;
            continue;
        }

        switch (expression[index]) {
        case '(':
            ++paren_depth;
            break;
        case ')':
            --paren_depth;
            break;
        case '{':
            ++brace_depth;
            break;
        case '}':
            --brace_depth;
            break;
        case ',':
            if (paren_depth == 0 && brace_depth == 0) {
                arguments.push_back(trim_copy(expression.substr(item_begin, index - item_begin)));
                item_begin = index + 1;
            }
            break;
        default:
            break;
        }
    }

    if (item_begin < close_paren_index || !arguments.empty()) {
        arguments.push_back(
            trim_copy(expression.substr(item_begin, close_paren_index - item_begin)));
    }

    return arguments;
}

std::string substitute_function_parameters(std::string_view expression,
                                           std::span<const std::string> parameter_names,
                                           std::span<const std::string> replacement_expressions) {
    std::string substituted;
    substituted.reserve(expression.size());

    std::size_t index = 0;
    while (index < expression.size()) {
        const std::size_t radix_end = consume_radix_literal(expression, index);
        if (radix_end != index) {
            substituted += std::string(expression.substr(index, radix_end - index));
            index = radix_end;
            continue;
        }

        const char ch = expression[index];
        if (!is_identifier_start(ch)) {
            substituted += ch;
            ++index;
            continue;
        }

        std::size_t end = index + 1;
        while (end < expression.size()) {
            if (is_identifier_char(expression[end])) {
                ++end;
                continue;
            }
            if (expression[end] == '.' && end + 1 < expression.size() &&
                is_identifier_start(expression[end + 1])) {
                end += 2;
                while (end < expression.size() && is_identifier_char(expression[end])) {
                    ++end;
                }
                continue;
            }
            break;
        }

        const std::string_view identifier = expression.substr(index, end - index);
        bool replaced = false;
        for (std::size_t parameter_index = 0; parameter_index < parameter_names.size();
             ++parameter_index) {
            if (identifier == parameter_names[parameter_index]) {
                substituted += '(';
                substituted += replacement_expressions[parameter_index];
                substituted += ')';
                replaced = true;
                break;
            }
        }
        if (!replaced) {
            substituted += std::string(identifier);
        }
        index = end;
    }

    return substituted;
}

}  // namespace console_calc::detail
