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
        {"x", console_calc::make_value_definition("pi + 1")},
        {"y", console_calc::make_value_definition("sin(x)")},
    };

    const auto value =
        console_calc::evaluate_expanded_expression(parser, "y", constants, variables, std::nullopt);
    const auto* scalar = std::get_if<double>(&value);
    return scalar != nullptr && *scalar < -0.84 && *scalar > -0.85;
}

bool expect_expanded_function_expression_helper() {
    console_calc::ExpressionParser parser;
    const console_calc::ConstantTable constants{
        {"pi", 3.14159265358979323846},
    };
    const console_calc::DefinitionTable definitions{
        {"f", console_calc::make_function_definition({"x"}, "x + 1")},
        {"vals", console_calc::make_value_definition("{1, 2, 3}")},
    };

    const auto direct_value =
        console_calc::evaluate_expanded_expression(parser, "f(3)", constants, definitions, std::nullopt);
    const auto nested_value = console_calc::evaluate_expanded_expression(
        parser, "f(f(3))", constants, definitions, std::nullopt);
    const auto mapped_value = console_calc::evaluate_expanded_expression(
        parser, "sum(map(vals, f(_)))", constants, definitions, std::nullopt);
    return std::holds_alternative<std::int64_t>(direct_value) &&
           std::get<std::int64_t>(direct_value) == 4 &&
           std::holds_alternative<std::int64_t>(nested_value) &&
           std::get<std::int64_t>(nested_value) == 5 &&
           std::holds_alternative<std::int64_t>(mapped_value) &&
           std::get<std::int64_t>(mapped_value) == 9;
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

    if (!expect_expanded_function_expression_helper()) {
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
