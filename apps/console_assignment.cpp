#include "console_assignment.h"

#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>
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

[[nodiscard]] bool is_braced_list_literal(std::string_view text) {
    if (text.size() < 2 || text.front() != '{' || text.back() != '}') {
        return false;
    }

    int depth = 0;
    for (std::size_t index = 0; index < text.size(); ++index) {
        if (text[index] == '{') {
            ++depth;
        } else if (text[index] == '}') {
            --depth;
            if (depth == 0 && index + 1 != text.size()) {
                return false;
            }
        }

        if (depth < 0) {
            return false;
        }
    }

    return depth == 0;
}

}  // namespace

std::optional<VariableAssignment> parse_variable_assignment(std::string_view text) {
    const std::size_t separator = text.find(':');
    if (separator == std::string_view::npos) {
        return std::nullopt;
    }

    const std::string name = trim(text.substr(0, separator));
    const std::string expression = trim(text.substr(separator + 1));
    if (!is_identifier(name)) {
        return std::nullopt;
    }
    if (expression.empty()) {
        throw std::invalid_argument("expected expression after ':'");
    }

    return VariableAssignment{name, expression};
}

VariableValue evaluate_assignment_value(const ExpressionParser& parser,
                                        std::string_view expression) {
    const std::string trimmed = trim(expression);
    if (is_braced_list_literal(trimmed)) {
        return parser.evaluate_value(trimmed);
    }

    const std::vector<std::string> items = split_top_level_items(trimmed);
    if (items.size() == 1) {
        return parser.evaluate_value(trimmed);
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
    return parser.evaluate_value(list_expression);
}

}  // namespace console_calc
