#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "console_calc/value.h"

namespace console_calc {

class ExpressionParser;

using ConstantTable = std::unordered_map<std::string, double>;
using VariableTable = std::unordered_map<std::string, std::string>;

[[nodiscard]] bool is_identifier(std::string_view text);
[[nodiscard]] bool is_braced_list_literal(std::string_view text);

[[nodiscard]] std::string expand_expression_identifiers(
    std::string_view expression, const ConstantTable& constants, const VariableTable& variables,
    const std::optional<Value>& result_reference);

[[nodiscard]] Value evaluate_expanded_expression(const ExpressionParser& parser,
                                                 std::string_view expression,
                                                 const ConstantTable& constants,
                                                 const VariableTable& variables,
                                                 const std::optional<Value>& result_reference);

[[nodiscard]] Value evaluate_expanded_expression(const ExpressionParser& parser,
                                                 std::string_view expanded_expression);

}  // namespace console_calc
