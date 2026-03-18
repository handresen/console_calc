#include <cstdlib>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "console_calc_app.h"

namespace {

constexpr std::string_view k_green_prompt = "\x1b[32m";
constexpr std::string_view k_color_reset = "\x1b[0m";

[[nodiscard]] std::string prompt(std::size_t depth) {
    return std::string(k_green_prompt) + std::to_string(depth) + '>' +
           std::string(k_color_reset);
}

bool expect_argument_mode_success() {
    const std::vector<std::string_view> args = {"(", "2", "+", "3", ")", "*", "4"};
    std::istringstream input;
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    return exit_code == 0 && output.str() == "20\n" && error.str().empty();
}

bool expect_argument_mode_failure() {
    const std::vector<std::string_view> args = {"1", "+"};
    std::istringstream input;
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    return exit_code == 1 && output.str().empty() && error.str() == "error: expected number after operator\n";
}

bool expect_console_mode_success() {
    const std::vector<std::string_view> args;
    std::istringstream input("1+1\n(2 + 3) * 4\ns\n+\ns\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + "2\n" +
        prompt(1) + "20\n" +
        prompt(2) + "0:2\n1:20\n" +
        prompt(2) + "22\n" +
        prompt(1) + "0:22\n" +
        prompt(1);
    return exit_code == 0 &&
           output.str() == expected_output &&
           error.str().empty();
}

bool expect_console_mode_recovery_after_error() {
    const std::vector<std::string_view> args;
    std::istringstream input("+\n1+\n  \n1+1\nQ\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + prompt(0) + prompt(0) + prompt(0) + "2\n" + prompt(1);
    return exit_code == 0 && output.str() == expected_output &&
           error.str() ==
               "error: stack requires at least two values\nerror: expected number after operator\n";
}

bool expect_console_mode_stack_limit() {
    const std::vector<std::string_view> args;
    std::istringstream input("1\n2\n3\n4\n5\n6\n7\n8\n9\n10\nq\n");
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
        prompt(6) + "7\n" +
        prompt(7) + "8\n" +
        prompt(8) + "9\n" +
        prompt(9) +
        prompt(9);
    return exit_code == 0 && output.str() == expected_output &&
           error.str() == "error: stack is full\n";
}

bool expect_console_mode_stack_commands() {
    const std::vector<std::string_view> args;
    std::istringstream input("1\n2\ndup\nswap\ns\ndrop\ns\nclear\ns\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + "1\n" +
        prompt(1) + "2\n" +
        prompt(2) +
        prompt(3) +
        prompt(3) + "0:1\n1:2\n2:2\n" +
        prompt(3) +
        prompt(2) + "0:1\n1:2\n" +
        prompt(2) +
        prompt(0) +
        prompt(0);
    return exit_code == 0 && output.str() == expected_output && error.str().empty();
}

bool expect_console_mode_stack_command_errors() {
    const std::vector<std::string_view> args;
    std::istringstream input("dup\ndrop\nswap\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output = prompt(0) + prompt(0) + prompt(0) + prompt(0);
    return exit_code == 0 && output.str() == expected_output &&
           error.str() ==
               "error: stack requires at least one value\n"
               "error: stack requires at least one value\n"
               "error: stack requires at least two values\n";
}

bool expect_console_mode_result_reference() {
    const std::vector<std::string_view> args;
    std::istringstream input("5\nr*2\nr+r\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) + "5\n" +
        prompt(1) + "10\n" +
        prompt(2) + "20\n" +
        prompt(3);
    return exit_code == 0 && output.str() == expected_output && error.str().empty();
}

bool expect_console_mode_result_reference_error() {
    const std::vector<std::string_view> args;
    std::istringstream input("r*2\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output = prompt(0) + prompt(0);
    return exit_code == 0 && output.str() == expected_output &&
           error.str() == "error: result reference requires at least one value\n";
}

}  // namespace

int main() {
    if (!expect_argument_mode_success()) {
        return EXIT_FAILURE;
    }

    if (!expect_argument_mode_failure()) {
        return EXIT_FAILURE;
    }

    if (!expect_console_mode_success()) {
        return EXIT_FAILURE;
    }

    if (!expect_console_mode_recovery_after_error()) {
        return EXIT_FAILURE;
    }

    if (!expect_console_mode_stack_limit()) {
        return EXIT_FAILURE;
    }

    if (!expect_console_mode_stack_commands()) {
        return EXIT_FAILURE;
    }

    if (!expect_console_mode_stack_command_errors()) {
        return EXIT_FAILURE;
    }

    if (!expect_console_mode_result_reference()) {
        return EXIT_FAILURE;
    }

    if (!expect_console_mode_result_reference_error()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
