#include <cmath>
#include <cstdlib>
#include <exception>
#include <string>
#include <utility>
#include <vector>

#include "console_calc/expression_parser.h"

namespace {

bool almost_equal(double lhs, double rhs) {
    return std::fabs(lhs - rhs) < 1e-12;
}

bool expect_value(console_calc::ExpressionParser& parser, const std::string& expression,
                  double expected) {
    try {
        return almost_equal(parser.evaluate(expression), expected);
    } catch (const std::exception&) {
        return false;
    }
}

bool expect_invalid(console_calc::ExpressionParser& parser, const std::string& expression) {
    try {
        (void)parser.evaluate(expression);
        return false;
    } catch (const std::invalid_argument&) {
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

}  // namespace

int main() {
    console_calc::ExpressionParser parser;

    const std::vector<std::pair<std::string, double>> valid_cases = {
        {"1+1", 2.0},
        {"2 + 3 * 4", 20.0},
        {"20 / 5 - 1", 3.0},
        {"1.5 + 2.25", 3.75},
        {".5 * 8", 4.0},
        {"1.3e2 / 2", 65.0},
        {"10. - 3.5 + .5", 7.0},
    };

    for (const auto& [expression, expected] : valid_cases) {
        if (!expect_value(parser, expression, expected)) {
            return EXIT_FAILURE;
        }
    }

    const std::vector<std::string> invalid_cases = {
        "",
        "+1",
        "1+",
        "1++2",
        "-3",
        ".",
        "1e",
        "1e+",
        "(1+2)",
        "2*-3",
        "2a+1",
    };

    for (const auto& expression : invalid_cases) {
        if (!expect_invalid(parser, expression)) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
