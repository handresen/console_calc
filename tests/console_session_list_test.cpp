#include "console_session_test_support.h"

#include <sstream>
#include <string_view>
#include <vector>

#include "console_test_utils.h"

namespace console_calc::test {
namespace {

using console_calc::test::expect_console_transcript;
using console_calc::test::prompt;

bool expect_console_mode_list_variables() {
    const std::vector<std::string_view> args;
    std::istringstream input("vals:1,2,3,4,5,7.0,sin(pi+0.1)\nsum(vals)\nvals:4,5\nsum(vals)\nq\n");
    std::ostringstream output;
    std::ostringstream error;
    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + prompt(0) + "21.9002\n" + prompt(1) + prompt(1) + "9\n" + prompt(2);
    return expect_console_transcript("console mode list variables", exit_code, 0, output.str(),
                                     expected_output, error.str(), "");
}

bool expect_console_mode_late_bound_list_variables() {
    const std::vector<std::string_view> args;
    std::istringstream input("xs:1,2,3\ntotal:sum(xs)\ntotal\nxs:4,5\ntotal\nq\n");
    std::ostringstream output;
    std::ostringstream error;
    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + prompt(0) + prompt(0) + "6\n" + prompt(1) + prompt(1) + "9\n" + prompt(2);
    return expect_console_transcript("console mode late bound list variables", exit_code, 0,
                                     output.str(), expected_output, error.str(), "");
}

bool expect_console_mode_list_stack_values() {
    const std::vector<std::string_view> args;
    std::istringstream input("{1,2,3}\ns\nq\n");
    std::ostringstream output;
    std::ostringstream error;
    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + "{1, 2, 3}\n" + prompt(1) + "0:{1, 2, 3}\n" + prompt(1);
    return expect_console_transcript("console mode list stack values", exit_code, 0,
                                     output.str(), expected_output, error.str(), "");
}

bool expect_console_mode_list_stack_operator_error() {
    const std::vector<std::string_view> args;
    std::istringstream input("{1,2,3}\n4\n+\ns\nq\n");
    std::ostringstream output;
    std::ostringstream error;
    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + "{1, 2, 3}\n" +
        prompt(1) + "4\n" +
        prompt(2) +
        prompt(2) + "0:{1, 2, 3}\n1:4\n" +
        prompt(2);
    return expect_console_transcript("console mode list stack operator error", exit_code, 0,
                                     output.str(), expected_output, error.str(),
                                     "error: stack operator requires scalar values\n");
}

bool expect_console_mode_list_result_reference() {
    const std::vector<std::string_view> args;
    std::istringstream input("{1,2,3}\nr\nsum(r)\nq\n");
    std::ostringstream output;
    std::ostringstream error;
    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + "{1, 2, 3}\n" + prompt(1) + "{1, 2, 3}\n" + prompt(2) + "6\n" + prompt(3);
    return expect_console_transcript("console mode list result reference", exit_code, 0,
                                     output.str(), expected_output, error.str(), "");
}

bool expect_console_mode_long_list_result_output() {
    const std::vector<std::string_view> args;
    std::istringstream input("range(1, 12)\nq\n");
    std::ostringstream output;
    std::ostringstream error;
    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + "{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, <hiding 2 entries>}\n" + prompt(1);
    return expect_console_transcript("console mode long list result output", exit_code, 0,
                                     output.str(), expected_output, error.str(), "");
}

bool expect_console_mode_long_list_stack_output() {
    const std::vector<std::string_view> args;
    std::istringstream input("range(1, 12)\ns\nq\n");
    std::ostringstream output;
    std::ostringstream error;
    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + "{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, <hiding 2 entries>}\n" +
        prompt(1) + "0:{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, <hiding 2 entries>}\n" +
        prompt(1);
    return expect_console_transcript("console mode long list stack output", exit_code, 0,
                                     output.str(), expected_output, error.str(), "");
}

bool expect_console_mode_map_inline_expression() {
    const std::vector<std::string_view> args;
    std::istringstream input("l:{1,2,3}\nmap(l,sin(_))\nsum(map({1,2,3},sin(_)))\nq\n");
    std::ostringstream output;
    std::ostringstream error;
    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) +
        prompt(0) + "{0.8414709848078965, 0.90929742682568171, 0.14112000805986721}\n" +
        prompt(1) + "1.89189\n" +
        prompt(2);
    return expect_console_transcript("console mode map inline expression", exit_code, 0,
                                     output.str(), expected_output, error.str(), "");
}

}  // namespace

bool expect_console_mode_list_behaviors() {
    return expect_console_mode_list_variables() &&
           expect_console_mode_late_bound_list_variables() &&
           expect_console_mode_list_stack_values() &&
           expect_console_mode_list_stack_operator_error() &&
           expect_console_mode_list_result_reference() &&
           expect_console_mode_long_list_result_output() &&
           expect_console_mode_long_list_stack_output() &&
           expect_console_mode_map_inline_expression();
}

}  // namespace console_calc::test
