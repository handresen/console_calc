#include "console_session_test_support.h"

#include <sstream>
#include <string_view>
#include <vector>

#include "console_test_utils.h"

namespace console_calc::test {
namespace {

using console_calc::test::expect_console_transcript;
using console_calc::test::prompt;

bool expect_console_mode_success() {
    const std::vector<std::string_view> args;
    std::istringstream input("sin(pi / 2)\n(2 + 3) * 4\ns\n+\ns\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + "1\n" +
        prompt(1) + "20\n" +
        prompt(2) + "0:1\n1:20\n" +
        prompt(2) + "21\n" +
        prompt(1) + "0:21\n" +
        prompt(1);
    return expect_console_transcript("console mode success", exit_code, 0, output.str(),
                                     expected_output, error.str(), "");
}

bool expect_console_mode_recovery_after_error() {
    const std::vector<std::string_view> args;
    std::istringstream input("+\n1+\n  \n1+1\nQ\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + prompt(0) + prompt(0) + prompt(0) + "2\n" + prompt(1);
    return expect_console_transcript("console mode recovery after error", exit_code, 0,
                                     output.str(), expected_output, error.str(),
                                     "error: stack requires at least two values\n"
                                     "error: expected number after operator\n");
}

bool expect_console_mode_stack_limit() {
    const std::vector<std::string_view> args;
    std::istringstream input("1\n2\n3\n4\n5\n6\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + "1\n" +
        prompt(1) + "2\n" +
        prompt(2) + "3\n" +
        prompt(3) + "4\n" +
        prompt(4) + "5\n" +
        prompt(5) + "6\n" +
        prompt(6);
    return expect_console_transcript("console mode stack limit", exit_code, 0, output.str(),
                                     expected_output, error.str(), "");
}

bool expect_console_mode_stack_commands() {
    const std::vector<std::string_view> args;
    std::istringstream input("1\n2\n3\n4\ndup\ns\ndrop\ns\nclear\ns\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + "1\n" +
        prompt(1) + "2\n" +
        prompt(2) + "3\n" +
        prompt(3) + "4\n" +
        prompt(4) +
        prompt(5) + "0:1\n1:2\n2:3\n3:4\n4:4\n" +
        prompt(5) +
        prompt(4) + "0:1\n1:2\n2:3\n3:4\n" +
        prompt(4) +
        prompt(0) +
        prompt(0);
    return expect_console_transcript("console mode stack commands", exit_code, 0, output.str(),
                                     expected_output, error.str(), "");
}

bool expect_console_mode_stack_command_errors() {
    const std::vector<std::string_view> args;
    std::istringstream input("dup\ndrop\nswap\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output = prompt(0) + prompt(0) + prompt(0) + prompt(0);
    return expect_console_transcript("console mode stack command errors", exit_code, 0,
                                     output.str(), expected_output, error.str(),
                                     "error: stack requires at least one value\n"
                                     "error: stack requires at least one value\n"
                                     "error: stack requires at least two values\n");
}

bool expect_console_mode_result_reference() {
    const std::vector<std::string_view> args;
    std::istringstream input("pi\nr*2\ne+r\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + "3.14159\n" +
        prompt(1) + "6.28319\n" +
        prompt(2) + "9.00147\n" +
        prompt(3);
    return expect_console_transcript("console mode result reference", exit_code, 0,
                                     output.str(), expected_output, error.str(), "");
}

bool expect_console_mode_integer_display_modes() {
    const std::vector<std::string_view> args;
    std::istringstream input("255\nhex\ns\nbin\ns\n1.5\ns\ndec\ns\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + "255\n" +
        prompt(1) +
        prompt(1) + "0:0xff\n" +
        prompt(1) +
        prompt(1) + "0:0b11111111\n" +
        prompt(1) + "1.5\n" +
        prompt(2) + "0:0b11111111\n1:1.5\n" +
        prompt(2) +
        prompt(2) + "0:255\n1:1.5\n" +
        prompt(2);
    return expect_console_transcript("console mode integer display modes", exit_code, 0,
                                     output.str(), expected_output, error.str(), "");
}

bool expect_console_mode_radix_literals() {
    const std::vector<std::string_view> args;
    std::istringstream input("0x10+5\n0b1010+1\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + "21\n" +
        prompt(1) + "11\n" +
        prompt(2);
    return expect_console_transcript("console mode radix literals", exit_code, 0, output.str(),
                                     expected_output, error.str(), "");
}

}  // namespace

bool expect_console_mode_basic_behaviors() {
    return expect_console_mode_success() &&
           expect_console_mode_recovery_after_error() &&
           expect_console_mode_stack_limit() &&
           expect_console_mode_stack_commands() &&
           expect_console_mode_stack_command_errors() &&
           expect_console_mode_result_reference() &&
           expect_console_mode_integer_display_modes() &&
           expect_console_mode_radix_literals();
}

}  // namespace console_calc::test
