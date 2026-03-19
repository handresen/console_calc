#include <cstdlib>
#include <sstream>
#include <string_view>
#include <vector>

#include "expression_environment.h"
#include "console_calc_app.h"
#include "console_calc/expression_parser.h"
#include "console_test_utils.h"

namespace {

using console_calc::test::expect_console_transcript;

bool expect_expanded_expression_helper() {
    console_calc::ExpressionParser parser;
    const console_calc::ConstantTable constants{
        {"pi", 3.14159265358979323846},
    };
    const console_calc::DefinitionTable variables{
        {"x", {"pi + 1"}},
        {"y", {"sin(x)"}},
    };

    const auto value =
        console_calc::evaluate_expanded_expression(parser, "y", constants, variables, std::nullopt);
    const auto* scalar = std::get_if<double>(&value);
    return scalar != nullptr && *scalar < -0.84 && *scalar > -0.85;
}

bool expect_argument_mode_success() {
    const std::vector<std::string_view> args = {"pow(2,", "3)", "+", "sin(pi", "/", "2)"};
    std::istringstream input;
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    return expect_console_transcript("argument mode success", exit_code, 0, output.str(), "9\n",
                                     error.str(), "");
}

bool expect_argument_mode_failure() {
    const std::vector<std::string_view> args = {"unknown_name", "+", "1"};
    std::istringstream input;
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    return expect_console_transcript("argument mode failure", exit_code, 1, output.str(), "",
                                     error.str(),
                                     "error: unknown identifier: unknown_name\n");
}

}  // namespace

int main() {
    if (!expect_expanded_expression_helper()) {
        return EXIT_FAILURE;
    }

    if (!expect_argument_mode_success()) {
        return EXIT_FAILURE;
    }

    if (!expect_argument_mode_failure()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
