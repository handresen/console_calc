#pragma once

#include "console_calc/expression_ast.h"

namespace console_calc {

[[nodiscard]] double evaluate_expression(const Expression& expression);

}  // namespace console_calc
