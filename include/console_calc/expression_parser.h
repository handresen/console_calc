#pragma once

#include <string>

#include "console_calc/expression_ast.h"

namespace console_calc {

class ExpressionParser {
public:
    [[nodiscard]] Expression parse(const std::string& expression) const;
    [[nodiscard]] double evaluate(const std::string& expression) const;
};

}  // namespace console_calc
