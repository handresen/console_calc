#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "expression_environment.h"

namespace console_calc {

class ExpressionParser;

struct UserAssignment {
    std::string name;
    std::vector<std::string> parameters;
    std::string expression;

    [[nodiscard]] bool is_function() const { return !parameters.empty(); }
};

[[nodiscard]] std::optional<UserAssignment> parse_user_assignment(std::string_view text);
[[nodiscard]] std::string normalize_assignment_expression(std::string_view expression);

}  // namespace console_calc
