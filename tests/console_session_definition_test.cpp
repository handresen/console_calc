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

}  // namespace

bool expect_console_mode_definition_behaviors() {
    return expect_console_mode_variables() &&
           expect_console_mode_late_bound_variables() &&
           expect_console_mode_circular_reference_error() &&
           expect_console_mode_self_circular_reference_error() &&
           expect_console_mode_variable_constant_conflict() &&
           expect_console_mode_result_reference_error() &&
           expect_console_mode_unknown_identifier_error();
}

}  // namespace console_calc::test
