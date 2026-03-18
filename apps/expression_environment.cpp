#include "expression_environment.h"

#include "console_calc/builtin_function.h"

#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace console_calc {

namespace {

[[nodiscard]] bool is_identifier_start(char ch) {
    return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

[[nodiscard]] bool is_identifier_char(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

[[nodiscard]] std::string format_number(double value) {
    std::ostringstream stream;
    stream << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
    return stream.str();
}

[[nodiscard]] std::string format_list_literal(const std::vector<double>& values) {
    std::string result = "{";
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            result += ", ";
        }
        result += format_number(values[index]);
    }
    result += "}";
    return result;
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

std::string expand_expression_identifiers(std::string_view expression,
                                          const ConstantTable& constants,
                                          const VariableTable& variables,
                                          const std::optional<double>& result_reference) {
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
            expanded += format_number(*result_reference);
        } else if (const auto found = variables.find(identifier); found != variables.end()) {
            if (const auto* scalar_value = std::get_if<double>(&found->second)) {
                expanded += format_number(*scalar_value);
            } else {
                expanded += format_list_literal(std::get<std::vector<double>>(found->second));
            }
        } else if (const auto found = constants.find(identifier); found != constants.end()) {
            expanded += format_number(found->second);
        } else {
            throw std::invalid_argument("unknown identifier: " + identifier);
        }

        index = end;
    }

    return expanded;
}

}  // namespace console_calc
