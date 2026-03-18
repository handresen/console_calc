#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "console_calc/value.h"

namespace console_calc {

class ExpressionParser;

using ConstantTable = std::unordered_map<std::string, double>;

struct UserDefinition {
    std::string expression;
};

using DefinitionTable = std::unordered_map<std::string, UserDefinition>;

[[nodiscard]] bool is_identifier(std::string_view text);
[[nodiscard]] bool is_braced_list_literal(std::string_view text);

[[nodiscard]] std::string expand_expression_identifiers(
    std::string_view expression, const ConstantTable& constants, const DefinitionTable& definitions,
    const std::optional<Value>& result_reference);

[[nodiscard]] Value evaluate_expanded_expression(const ExpressionParser& parser,
                                                 std::string_view expression,
                                                 const ConstantTable& constants,
                                                 const DefinitionTable& definitions,
                                                 const std::optional<Value>& result_reference);

[[nodiscard]] Value evaluate_expanded_expression(const ExpressionParser& parser,
                                                 std::string_view expanded_expression);

}  // namespace console_calc
