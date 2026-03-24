#include "expression_environment.h"

#include "console_calc/builtin_function.h"
#include "console_calc/expression_parser.h"
#include "console_calc/special_form.h"
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

[[nodiscard]] bool is_blank_text(std::string_view text) {
    return skip_whitespace(text, 0) == text.size();
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

[[nodiscard]] bool is_inside_placeholder_expression(const std::vector<ExpansionFrame>& frames) {
    for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
        if (it->kind == ExpansionFrameKind::call &&
            (it->identifier == "map" || it->identifier == "map_at" ||
             it->identifier == "list_where") &&
            it->argument_index == 1) {
            return true;
        }
    }

    return false;
}

[[nodiscard]] bool is_builtin_or_special_call(std::string_view identifier, bool followed_by_call) {
    return followed_by_call &&
           (is_builtin_function_name(identifier) || is_special_form_name(identifier));
}

[[nodiscard]] std::size_t call_open_paren_index(std::string_view expression, std::size_t index) {
    const std::size_t open_paren = skip_whitespace(expression, index);
    return open_paren < expression.size() && expression[open_paren] == '('
               ? open_paren
               : std::string_view::npos;
}

[[nodiscard]] std::size_t find_call_close_paren(std::string_view expression,
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

[[nodiscard]] std::string extract_unary_call_argument(std::string_view expression,
                                                      std::size_t open_paren_index,
                                                      std::size_t close_paren_index,
                                                      std::string_view identifier) {
    std::size_t argument_begin = open_paren_index + 1;
    int paren_depth = 0;
    int brace_depth = 0;
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
                throw std::invalid_argument("function '" + std::string(identifier) +
                                            "' expects 1 argument");
            }
            break;
        default:
            break;
        }
    }

    const std::string argument =
        std::string(expression.substr(argument_begin, close_paren_index - argument_begin));
    if (is_blank_text(argument)) {
        throw std::invalid_argument("function '" + std::string(identifier) +
                                    "' expects 1 argument");
    }
    return argument;
}

std::string expand_expression_identifiers_impl(
    std::string_view expression, const ConstantTable& constants,
    const DefinitionTable& definitions, const std::optional<Value>& result_reference,
    std::unordered_set<std::string>& expansion_stack,
    bool allow_placeholder_identifier = false);

[[nodiscard]] std::string expand_value_definition_expression(
    const UserDefinition& definition, const ConstantTable& constants,
    const DefinitionTable& definitions, const std::optional<Value>& result_reference,
    std::unordered_set<std::string>& expansion_stack) {
    if (!is_value_definition(definition)) {
        throw std::invalid_argument("function definitions are not supported in expressions yet");
    }

    return expand_expression_identifiers_impl(as_value_definition(definition).expression, constants,
                                              definitions, result_reference, expansion_stack);
}

[[nodiscard]] std::string substitute_function_parameter(
    std::string_view expression, std::string_view parameter_name,
    std::string_view replacement_expression) {
    std::string substituted;
    substituted.reserve(expression.size() + replacement_expression.size());

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
        while (end < expression.size() && is_identifier_char(expression[end])) {
            ++end;
        }

        const std::string_view identifier = expression.substr(index, end - index);
        if (identifier == parameter_name) {
            substituted += '(';
            substituted += replacement_expression;
            substituted += ')';
        } else {
            substituted += std::string(identifier);
        }
        index = end;
    }

    return substituted;
}

[[nodiscard]] std::string expand_function_definition_call_expression(
    std::string_view identifier, const UserDefinition& definition, std::string_view raw_argument,
    const ConstantTable& constants, const DefinitionTable& definitions,
    const std::optional<Value>& result_reference,
    std::unordered_set<std::string>& expansion_stack,
    bool allow_placeholder_identifier) {
    if (is_value_definition(definition)) {
        throw std::invalid_argument("definition is not callable: " + std::string(identifier));
    }

    const auto& function = as_function_definition(definition);
    if (function.parameters.size() != 1) {
        throw std::invalid_argument("function '" + std::string(identifier) +
                                    "' expects 1 argument");
    }

    const std::string expanded_argument =
        expand_expression_identifiers_impl(raw_argument, constants, definitions, result_reference,
                                           expansion_stack, allow_placeholder_identifier);
    const std::string substituted_body = substitute_function_parameter(
        function.expression, function.parameters[0], expanded_argument);
    const std::string name(identifier);
    if (!expansion_stack.insert(name).second) {
        throw std::invalid_argument("circular variable reference: " + name);
    }
    const std::string expanded_body = expand_expression_identifiers_impl(
        substituted_body, constants, definitions, result_reference, expansion_stack,
        allow_placeholder_identifier);
    expansion_stack.erase(name);
    return expanded_body;
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
    const std::optional<Value>& result_reference, std::unordered_set<std::string>& expansion_stack,
    bool allow_placeholder_identifier) {
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

        if (identifier == "_" &&
            (allow_placeholder_identifier || is_inside_placeholder_expression(frames))) {
            expanded += identifier;
            pending_call_identifier.reset();
        } else if (is_builtin_or_special_call(identifier, followed_by_call)) {
            expanded += identifier;
            pending_call_identifier = followed_by_call ? std::optional<std::string>(identifier)
                                                       : std::nullopt;
        } else if (followed_by_call && definitions.contains(identifier) &&
                   !is_value_definition(definitions.at(identifier))) {
            const auto& definition = definitions.at(identifier);
            const std::size_t open_paren_index = call_open_paren_index(expression, end);
            const std::size_t close_paren_index =
                find_call_close_paren(expression, open_paren_index);
            const std::string raw_argument = extract_unary_call_argument(
                expression, open_paren_index, close_paren_index, identifier);
            const bool placeholder_context =
                allow_placeholder_identifier || is_inside_placeholder_expression(frames);
            expanded += '(';
            expanded += expand_function_definition_call_expression(
                identifier, definition, raw_argument, constants, definitions, result_reference,
                expansion_stack, placeholder_context);
            expanded += ')';
            pending_call_identifier.reset();
            index = close_paren_index + 1;
            continue;
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

            const std::string variable_expression = expand_value_definition_expression(
                found->second, constants, definitions, result_reference, expansion_stack);
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
