#pragma once

#include <string_view>

#include "console_calc/expression_ast.h"

namespace console_calc {

[[nodiscard]] Expression parse_expression(std::string_view input);

}  // namespace console_calc
