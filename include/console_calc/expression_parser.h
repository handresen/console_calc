#pragma once

#include <string>

namespace console_calc {

class ExpressionParser {
public:
    [[nodiscard]] double evaluate(const std::string& expression) const;
};

}  // namespace console_calc
