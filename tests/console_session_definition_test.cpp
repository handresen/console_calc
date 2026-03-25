#include "console_session_test_support.h"

#include <sstream>
#include <string_view>
#include <vector>

#include "console_test_utils.h"

namespace console_calc::test {
namespace {

using console_calc::test::expect_console_transcript;
using console_calc::test::prompt;

bool expect_console_mode_variables() {
    const std::vector<std::string_view> args;
    std::istringstream input("x:7.0\nx*2\nx:pi\nx+r\nq\n");
    std::ostringstream output;
    std::ostringstream error;
    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) +
        prompt(0) + "14\n" +
        prompt(1) +
        prompt(1) + "17.1416\n" +
        prompt(2);
    return expect_console_transcript("console mode variables", exit_code, 0, output.str(),
                                     expected_output, error.str(), "");
}

bool expect_console_mode_late_bound_variables() {
    const std::vector<std::string_view> args;
    std::istringstream input("x:pi+1\nsx:sin(x)\nsx\nx:0\nsx\nq\n");
    std::ostringstream output;
    std::ostringstream error;
    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + prompt(0) + prompt(0) + "-0.841471\n" + prompt(1) + prompt(1) + "0\n" +
        prompt(2);
    return expect_console_transcript("console mode late bound variables", exit_code, 0,
                                     output.str(), expected_output, error.str(), "");
}

bool expect_console_mode_circular_reference_error() {
    const std::vector<std::string_view> args;
    std::istringstream input("a:1\nb:a+1\na:b+1\na\nq\n");
    std::ostringstream output;
    std::ostringstream error;
    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + prompt(0) + prompt(0) + prompt(0) + "1\n" + prompt(1);
    return expect_console_transcript("console mode circular reference error", exit_code, 0,
                                     output.str(), expected_output, error.str(),
                                     "error: circular variable reference: b\n");
}

bool expect_console_mode_self_circular_reference_error() {
    const std::vector<std::string_view> args;
    std::istringstream input("a:a+1\na\nq\n");
    std::ostringstream output;
    std::ostringstream error;
    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output = prompt(0) + prompt(0) + prompt(0);
    return expect_console_transcript("console mode self circular reference error", exit_code, 0,
                                     output.str(), expected_output, error.str(),
                                     "error: circular variable reference: a\n"
                                     "error: unknown identifier: a\n");
}

bool expect_console_mode_variable_constant_conflict() {
    const std::vector<std::string_view> args;
    std::istringstream input("pi:7\nq\n");
    std::ostringstream output;
    std::ostringstream error;
    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output = prompt(0) + prompt(0);
    return expect_console_transcript("console mode variable constant conflict", exit_code, 0,
                                     output.str(), expected_output, error.str(),
                                     "error: cannot redefine constant: pi\n");
}

bool expect_console_mode_result_reference_error() {
    const std::vector<std::string_view> args;
    std::istringstream input("r*2\nq\n");
    std::ostringstream output;
    std::ostringstream error;
    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output = prompt(0) + prompt(0);
    return expect_console_transcript("console mode result reference error", exit_code, 0,
                                     output.str(), expected_output, error.str(),
                                     "error: result reference requires at least one value\n");
}

bool expect_console_mode_unknown_identifier_error() {
    const std::vector<std::string_view> args;
    std::istringstream input("foo+1\nq\n");
    std::ostringstream output;
    std::ostringstream error;
    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output = prompt(0) + prompt(0);
    return expect_console_transcript("console mode unknown identifier error", exit_code, 0,
                                     output.str(), expected_output, error.str(),
                                     "error: unknown identifier: foo\n");
}

bool expect_console_mode_user_functions() {
    const std::vector<std::string_view> args;
    std::istringstream input(
        "f(x):x+1\npair_sum(x,y):x+y\nf(3)\nf(f(3))\npair_sum(2,5)\nvals:1,2,3\nsum(map(vals,f(_)))\nq\n");
    std::ostringstream output;
    std::ostringstream error;
    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + prompt(0) + prompt(0) + "4\n" + prompt(1) + "5\n" + prompt(2) + "7\n" +
        prompt(3) + prompt(3) + "9\n" + prompt(4);
    return expect_console_transcript("console mode user functions", exit_code, 0,
                                     output.str(), expected_output, error.str(), "");
}

bool expect_console_mode_echoed_value_assignments() {
    const std::vector<std::string_view> args;
    std::istringstream input("#x:pi+1\n#f(x):x+1\nq\n");
    std::ostringstream output;
    std::ostringstream error;
    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + "4.14159\n" + prompt(1) + prompt(1);
    return expect_console_transcript("console mode echoed value assignments", exit_code, 0,
                                     output.str(), expected_output, error.str(),
                                     "error: '#' is only supported for value assignments\n");
}

}  // namespace

bool expect_console_mode_definition_behaviors() {
    return expect_console_mode_variables() &&
           expect_console_mode_late_bound_variables() &&
           expect_console_mode_circular_reference_error() &&
           expect_console_mode_self_circular_reference_error() &&
           expect_console_mode_variable_constant_conflict() &&
           expect_console_mode_result_reference_error() &&
           expect_console_mode_unknown_identifier_error() &&
           expect_console_mode_user_functions() &&
           expect_console_mode_echoed_value_assignments();
}

}  // namespace console_calc::test
