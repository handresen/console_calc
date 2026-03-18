#include "expression_environment.h"

#include "console_calc/builtin_function.h"
#include "console_calc/value_format.h"

#include <cctype>
#include <string_view>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace console_calc {

namespace {

[[nodiscard]] bool is_identifier_start(char ch) {
    return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

[[nodiscard]] bool is_identifier_char(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

[[nodiscard]] bool is_followed_by_call(std::string_view expression, std::size_t index) {
    while (index < expression.size() &&
           std::isspace(static_cast<unsigned char>(expression[index]))) {
        ++index;
    }

    return index < expression.size() && expression[index] == '(';
}

}  // namespace

bool is_identifier(std::string_view text) {
    if (text.empty() || !is_identifier_start(text.front())) {
        return false;
    }

    for (std::size_t index = 1; index < text.size(); ++index) {
        if (!is_identifier_char(text[index])) {
            return false;
        }
    }

    return true;
}

bool is_braced_list_literal(std::string_view text) {
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

namespace {

std::string expand_expression_identifiers_impl(
    std::string_view expression, const ConstantTable& constants, const VariableTable& variables,
    const std::optional<Value>& result_reference, std::unordered_set<std::string>& expansion_stack) {
    std::string expanded;
    expanded.reserve(expression.size());

    std::size_t index = 0;
    while (index < expression.size()) {
        const char ch = expression[index];
        if (!is_identifier_start(ch)) {
            expanded += ch;
            ++index;
            continue;
        }

        std::size_t end = index + 1;
        while (end < expression.size() && is_identifier_char(expression[end])) {
            ++end;
        }

        const std::string identifier(expression.substr(index, end - index));
        if (is_builtin_function_name(identifier) && is_followed_by_call(expression, end)) {
            expanded += identifier;
        } else if (identifier == "r") {
            if (!result_reference.has_value()) {
                throw std::invalid_argument("result reference requires at least one value");
            }
            expanded += format_value(*result_reference);
        } else if (const auto found = variables.find(identifier); found != variables.end()) {
            if (!expansion_stack.insert(identifier).second) {
                throw std::invalid_argument("circular variable reference: " + identifier);
            }

            const std::string variable_expression = expand_expression_identifiers_impl(
                found->second, constants, variables, result_reference, expansion_stack);
            expansion_stack.erase(identifier);

            if (is_braced_list_literal(variable_expression)) {
                expanded += variable_expression;
            } else {
                expanded += '(';
                expanded += variable_expression;
                expanded += ')';
            }
        } else if (const auto found = constants.find(identifier); found != constants.end()) {
            expanded += format_scalar(found->second);
        } else {
            throw std::invalid_argument("unknown identifier: " + identifier);
        }

        index = end;
    }

    return expanded;
}

}  // namespace

std::string expand_expression_identifiers(std::string_view expression,
                                          const ConstantTable& constants,
                                          const VariableTable& variables,
                                          const std::optional<Value>& result_reference) {
    std::unordered_set<std::string> expansion_stack;
    return expand_expression_identifiers_impl(
        expression, constants, variables, result_reference, expansion_stack);
}

}  // namespace console_calc
