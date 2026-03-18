#pragma once

#include <optional>
#include <variant>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace console_calc {

using ConstantTable = std::unordered_map<std::string, double>;
using VariableValue = std::variant<double, std::vector<double>>;
using VariableTable = std::unordered_map<std::string, VariableValue>;

[[nodiscard]] bool is_identifier(std::string_view text);
[[nodiscard]] bool is_builtin_function_name(std::string_view text);

[[nodiscard]] std::string expand_expression_identifiers(
    std::string_view expression, const ConstantTable& constants, const VariableTable& variables,
    const std::optional<double>& result_reference);

}  // namespace console_calc
