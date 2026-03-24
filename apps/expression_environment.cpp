#include "expression_environment.h"

#include "expression_expansion_support.h"
#include "console_calc/expression_parser.h"
#include "console_calc/value_format.h"

#include <stdexcept>
#include <string>
#include <unordered_set>

namespace console_calc {

namespace {

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

[[nodiscard]] std::string expand_function_definition_call_expression(
    std::string_view identifier, const UserDefinition& definition,
    std::span<const std::string> raw_arguments,
    const ConstantTable& constants, const DefinitionTable& definitions,
    const std::optional<Value>& result_reference,
    std::unordered_set<std::string>& expansion_stack,
    bool allow_placeholder_identifier) {
    if (is_value_definition(definition)) {
        throw std::invalid_argument("definition is not callable: " + std::string(identifier));
    }

    const auto& function = as_function_definition(definition);
    if (function.parameters.size() != raw_arguments.size()) {
        throw std::invalid_argument("function '" + std::string(identifier) + "' expects " +
                                    std::to_string(function.parameters.size()) + " arguments");
    }

    std::vector<std::string> expanded_arguments;
    expanded_arguments.reserve(raw_arguments.size());
    for (const auto& raw_argument : raw_arguments) {
        if (detail::is_blank_text(raw_argument)) {
            throw std::invalid_argument("function '" + std::string(identifier) +
                                        "' received an empty argument");
        }
        expanded_arguments.push_back(expand_expression_identifiers_impl(
            raw_argument, constants, definitions, result_reference, expansion_stack,
            allow_placeholder_identifier));
    }
    const std::string substituted_body = detail::substitute_function_parameters(
        function.expression, function.parameters, expanded_arguments);
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
    if (text.empty() || !detail::is_identifier_start(text.front())) {
        return false;
    }

    for (std::size_t index = 1; index < text.size(); ++index) {
        if (!detail::is_identifier_char(text[index])) {
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
    std::vector<detail::ExpansionFrame> frames;
    std::optional<std::string> pending_call_identifier;

    std::size_t index = 0;
    while (index < expression.size()) {
        const char ch = expression[index];
        const std::size_t radix_end = detail::consume_radix_literal(expression, index);
        if (radix_end != index) {
            expanded += std::string(expression.substr(index, radix_end - index));
            pending_call_identifier.reset();
            index = radix_end;
            continue;
        }

        if (ch == '(') {
            expanded += ch;
            if (pending_call_identifier.has_value()) {
                frames.push_back({detail::ExpansionFrameKind::call, *pending_call_identifier, 0});
                pending_call_identifier.reset();
            } else {
                frames.push_back({detail::ExpansionFrameKind::group, {}, 0});
            }
            ++index;
            continue;
        }

        if (ch == '{') {
            expanded += ch;
            frames.push_back({detail::ExpansionFrameKind::list, {}, 0});
            pending_call_identifier.reset();
            ++index;
            continue;
        }

        if (ch == ',' && !frames.empty()) {
            expanded += ch;
            if (frames.back().kind == detail::ExpansionFrameKind::call) {
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

        if (!detail::is_identifier_start(ch)) {
            expanded += ch;
            pending_call_identifier.reset();
            ++index;
            continue;
        }

        std::size_t end = index + 1;
        while (end < expression.size()) {
            if (detail::is_identifier_char(expression[end])) {
                ++end;
                continue;
            }
            if (expression[end] == '.' && end + 1 < expression.size() &&
                detail::is_identifier_start(expression[end + 1])) {
                end += 2;
                while (end < expression.size() && detail::is_identifier_char(expression[end])) {
                    ++end;
                }
                continue;
            }
            break;
        }
        const std::string identifier(expression.substr(index, end - index));
        const bool followed_by_call = detail::is_followed_by_call(expression, end);

        if (identifier == "_" &&
            (allow_placeholder_identifier ||
             detail::is_inside_placeholder_expression(frames))) {
            expanded += identifier;
            pending_call_identifier.reset();
        } else if (detail::is_builtin_or_special_call(identifier, followed_by_call)) {
            expanded += identifier;
            pending_call_identifier = followed_by_call ? std::optional<std::string>(identifier)
                                                       : std::nullopt;
        } else if (followed_by_call && definitions.contains(identifier) &&
                   !is_value_definition(definitions.at(identifier))) {
            const auto& definition = definitions.at(identifier);
            const std::size_t open_paren_index =
                detail::call_open_paren_index(expression, end);
            const std::size_t close_paren_index =
                detail::find_call_close_paren(expression, open_paren_index);
            const std::vector<std::string> raw_arguments = detail::extract_call_arguments(
                expression, open_paren_index, close_paren_index);
            const bool placeholder_context =
                allow_placeholder_identifier ||
                detail::is_inside_placeholder_expression(frames);
            expanded += '(';
            expanded += expand_function_definition_call_expression(
                identifier, definition, raw_arguments, constants, definitions, result_reference,
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
