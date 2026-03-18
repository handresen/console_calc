#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "console_calc/value.h"

namespace console_calc {

using ConstantTable = std::unordered_map<std::string, double>;
using VariableValue = Value;
using VariableTable = std::unordered_map<std::string, VariableValue>;

[[nodiscard]] bool is_identifier(std::string_view text);

[[nodiscard]] std::string expand_expression_identifiers(
    std::string_view expression, const ConstantTable& constants, const VariableTable& variables,
    const std::optional<double>& result_reference);

}  // namespace console_calc
