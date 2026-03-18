#pragma once

#include <string>
#include <vector>

namespace console_calc::test {

struct ExpressionCase {
    std::string expression;
    bool expect_invalid = false;
    double expected_value = 0.0;
};

[[nodiscard]] std::vector<ExpressionCase> load_expression_cases(const std::string& directory);

}  // namespace console_calc::test
