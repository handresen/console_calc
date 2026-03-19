#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "console_calc/session_environment.h"
#include "console_calc/value.h"

namespace console_calc {

class ExpressionParser;

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
