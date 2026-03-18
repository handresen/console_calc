#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "expression_environment.h"

namespace console_calc {

class ExpressionParser;

struct VariableAssignment {
    std::string name;
    std::string expression;
};

[[nodiscard]] std::optional<VariableAssignment> parse_variable_assignment(std::string_view text);
[[nodiscard]] VariableValue evaluate_assignment_value(const ExpressionParser& parser,
                                                      std::string_view expression);

}  // namespace console_calc
