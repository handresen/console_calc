#pragma once

#include "console_calc/expression_ast.h"
#include "console_calc/value.h"

namespace console_calc {

[[nodiscard]] Value evaluate_expression(const Expression& expression);
[[nodiscard]] double evaluate_scalar_expression(const Expression& expression);

}  // namespace console_calc
