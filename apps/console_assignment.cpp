#include "console_assignment.h"

#include <cctype>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "console_calc/expression_parser.h"

namespace console_calc {

namespace {

[[nodiscard]] std::string trim(std::string_view text) {
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

[[nodiscard]] std::vector<std::string> split_top_level_items(std::string_view text);

[[nodiscard]] std::optional<UserAssignment> parse_assignment_target(std::string_view text) {
    const std::string trimmed = trim(text);
    if (trimmed.empty()) {
        return std::nullopt;
    }

    if (const std::size_t left_paren = trimmed.find('('); left_paren != std::string::npos) {
        if (trimmed.back() != ')') {
            return std::nullopt;
        }
        const std::string name = trim(std::string_view(trimmed).substr(0, left_paren));
        const std::string parameters_text = trim(std::string_view(trimmed).substr(
            left_paren + 1, trimmed.size() - left_paren - 2));
        if (!is_identifier(name)) {
            return std::nullopt;
        }
        const std::vector<std::string> parameters = split_top_level_items(parameters_text);
        if (parameters.empty()) {
            return std::nullopt;
        }
        std::unordered_set<std::string> seen_parameters;
        for (const auto& parameter : parameters) {
            if (!is_identifier(parameter) ||
                !seen_parameters.insert(parameter).second) {
                return std::nullopt;
            }
        }
        return UserAssignment{
            .name = name,
            .parameters = parameters,
            .expression = {},
        };
    }

    if (!is_identifier(trimmed)) {
        return std::nullopt;
    }

    return UserAssignment{
        .name = trimmed,
        .parameters = {},
        .expression = {},
    };
}

[[nodiscard]] std::vector<std::string> split_top_level_items(std::string_view text) {
    std::vector<std::string> items;
    std::size_t item_begin = 0;
    int paren_depth = 0;
    int brace_depth = 0;

    for (std::size_t index = 0; index < text.size(); ++index) {
        switch (text[index]) {
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
                items.push_back(trim(text.substr(item_begin, index - item_begin)));
                item_begin = index + 1;
            }
            break;
        default:
            break;
        }
    }

    items.push_back(trim(text.substr(item_begin)));
    return items;
}

}  // namespace

std::optional<UserAssignment> parse_user_assignment(std::string_view text) {
    const std::size_t separator = text.find(':');
    if (separator == std::string_view::npos) {
        return std::nullopt;
    }

    auto assignment = parse_assignment_target(text.substr(0, separator));
    const std::string expression = trim(text.substr(separator + 1));
    if (!assignment.has_value()) {
        return std::nullopt;
    }
    if (expression.empty()) {
        throw std::invalid_argument("expected expression after ':'");
    }

    assignment->expression = expression;
    return assignment;
}

std::string normalize_assignment_expression(std::string_view expression) {
    const std::string trimmed = trim(expression);
    if (is_braced_list_literal(trimmed)) {
        return trimmed;
    }

    const std::vector<std::string> items = split_top_level_items(trimmed);
    if (items.size() == 1) {
        return trimmed;
    }

    std::string list_expression = "{";
    for (std::size_t index = 0; index < items.size(); ++index) {
        if (items[index].empty()) {
            throw std::invalid_argument("expected expression after ','");
        }
        if (index != 0) {
            list_expression += ", ";
        }
        list_expression += items[index];
    }
    list_expression += '}';
    return list_expression;
}

}  // namespace console_calc
