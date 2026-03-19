#include "expression_environment.h"

#include "console_calc/builtin_function.h"
#include "console_calc/expression_parser.h"
#include "console_calc/value_format.h"

#include <cctype>
#include <string_view>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace console_calc {

namespace {

enum class ExpansionFrameKind {
    group,
    list,
    call,
};

struct ExpansionFrame {
    ExpansionFrameKind kind = ExpansionFrameKind::group;
    std::string identifier;
    std::size_t argument_index = 0;
};

[[nodiscard]] bool is_identifier_start(char ch) {
    return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

[[nodiscard]] bool is_identifier_char(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

[[nodiscard]] std::size_t skip_whitespace(std::string_view expression, std::size_t index) {
    while (index < expression.size() &&
           std::isspace(static_cast<unsigned char>(expression[index]))) {
        ++index;
    }

    return index;
}

[[nodiscard]] bool is_followed_by_call(std::string_view expression, std::size_t index) {
    index = skip_whitespace(expression, index);
    return index < expression.size() && expression[index] == '(';
}

[[nodiscard]] bool is_radix_digit(char ch, int base) {
    return base == 16 ? std::isxdigit(static_cast<unsigned char>(ch)) != 0 : (ch == '0' || ch == '1');
}

[[nodiscard]] std::size_t consume_radix_literal(std::string_view expression, std::size_t index) {
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

[[nodiscard]] bool is_inside_map_expression(const std::vector<ExpansionFrame>& frames) {
    for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
        if (it->kind == ExpansionFrameKind::call && it->identifier == "map" &&
            it->argument_index == 1) {
            return true;
        }
    }

    return false;
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
    std::string_view expression, const ConstantTable& constants,
    const DefinitionTable& definitions,
    const std::optional<Value>& result_reference, std::unordered_set<std::string>& expansion_stack) {
    std::string expanded;
    expanded.reserve(expression.size());
    std::vector<ExpansionFrame> frames;
    std::optional<std::string> pending_call_identifier;

    std::size_t index = 0;
    while (index < expression.size()) {
        const char ch = expression[index];
        const std::size_t radix_end = consume_radix_literal(expression, index);
        if (radix_end != index) {
            expanded += std::string(expression.substr(index, radix_end - index));
            pending_call_identifier.reset();
            index = radix_end;
            continue;
        }

        if (ch == '(') {
            expanded += ch;
            if (pending_call_identifier.has_value()) {
                frames.push_back({ExpansionFrameKind::call, *pending_call_identifier, 0});
                pending_call_identifier.reset();
            } else {
                frames.push_back({ExpansionFrameKind::group, {}, 0});
            }
            ++index;
            continue;
        }

        if (ch == '{') {
            expanded += ch;
            frames.push_back({ExpansionFrameKind::list, {}, 0});
            pending_call_identifier.reset();
            ++index;
            continue;
        }

        if (ch == ',' && !frames.empty()) {
            expanded += ch;
            if (frames.back().kind == ExpansionFrameKind::call) {
                ++frames.back().argument_index;
            }
            pending_call_identifier.reset();
            ++index;
            continue;
        }

        if (ch == ')' || ch == '}') {
            expanded += ch;
            if (!frames.empty()) {
                frames.pop_back();
            }
            pending_call_identifier.reset();
            ++index;
            continue;
        }

        if (!is_identifier_start(ch)) {
            expanded += ch;
            pending_call_identifier.reset();
            ++index;
            continue;
        }

        std::size_t end = index + 1;
        while (end < expression.size() && is_identifier_char(expression[end])) {
            ++end;
        }
        const std::string identifier(expression.substr(index, end - index));
        const bool followed_by_call = is_followed_by_call(expression, end);

        if (identifier == "_" && is_inside_map_expression(frames)) {
            expanded += identifier;
            pending_call_identifier.reset();
        } else if (is_builtin_function_name(identifier) && followed_by_call) {
            expanded += identifier;
            pending_call_identifier = followed_by_call ? std::optional<std::string>(identifier)
                                                       : std::nullopt;
        } else if (identifier == "r") {
            if (!result_reference.has_value()) {
                throw std::invalid_argument("result reference requires at least one value");
            }
            expanded += format_value(*result_reference);
            pending_call_identifier.reset();
        } else if (const auto found = definitions.find(identifier); found != definitions.end()) {
            if (!expansion_stack.insert(identifier).second) {
                throw std::invalid_argument("circular variable reference: " + identifier);
            }

            const std::string variable_expression = expand_expression_identifiers_impl(
                found->second.expression, constants, definitions, result_reference, expansion_stack);
            expansion_stack.erase(identifier);

            if (is_braced_list_literal(variable_expression)) {
                expanded += variable_expression;
            } else {
                expanded += '(';
                expanded += variable_expression;
                expanded += ')';
            }
            pending_call_identifier.reset();
        } else if (const auto found = constants.find(identifier); found != constants.end()) {
            expanded += format_scalar(found->second);
            pending_call_identifier.reset();
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
                                          const DefinitionTable& definitions,
                                          const std::optional<Value>& result_reference) {
    std::unordered_set<std::string> expansion_stack;
    return expand_expression_identifiers_impl(
        expression, constants, definitions, result_reference, expansion_stack);
}

Value evaluate_expanded_expression(const ExpressionParser& parser, std::string_view expression,
                                   const ConstantTable& constants,
                                   const DefinitionTable& definitions,
                                   const std::optional<Value>& result_reference) {
    return evaluate_expanded_expression(
        parser, expand_expression_identifiers(expression, constants, definitions, result_reference));
}

Value evaluate_expanded_expression(const ExpressionParser& parser,
                                   std::string_view expanded_expression) {
    return parser.evaluate_value(std::string(expanded_expression));
}

}  // namespace console_calc
