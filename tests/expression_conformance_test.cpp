#include <cstdlib>

#include "expression_case_loader.h"
#include "expression_conformance_checks.h"
#include "expression_conformance_test_support.h"

int main() {
    console_calc::ExpressionParser parser;

    if (!console_calc::test::run_conformance_check(
            "expect_expression_value_api",
            [&] { return console_calc::test::expect_expression_value_api(parser); }) ||
        !console_calc::test::run_conformance_check(
            "expect_expression_ast_shape",
            [&] { return console_calc::test::expect_expression_ast_shape(parser); }) ||
        !console_calc::test::run_conformance_check(
            "expect_expression_semantics",
            [&] { return console_calc::test::expect_expression_semantics(parser); }) ||
        !console_calc::test::run_conformance_check(
            "expect_expression_function_signature_errors",
            [&] { return console_calc::test::expect_expression_function_signature_errors(parser); })) {
        return EXIT_FAILURE;
    }

    const auto expression_cases =
        console_calc::test::load_expression_cases(CONSOLE_CALC_TEST_DATA_DIR);
    for (const auto& expression_case : expression_cases) {
        if (expression_case.expect_invalid) {
            if (!console_calc::test::expect_invalid(parser, expression_case.expression)) {
                return EXIT_FAILURE;
            }
            continue;
        }

        if (!console_calc::test::expect_value(parser, expression_case.expression,
                                              expression_case.expected_value)) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
