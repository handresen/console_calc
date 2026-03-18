#include <cstdlib>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "console_calc_app.h"

namespace {

bool expect_argument_mode_success() {
    const std::vector<std::string_view> args = {"2", "+", "3", "*", "4"};
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
    std::istringstream input("1+1\n2 + 3 * 4\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    return exit_code == 0 && output.str() == "2\n20\n" && error.str().empty();
}

bool expect_console_mode_recovery_after_error() {
    const std::vector<std::string_view> args;
    std::istringstream input("1+\n  \n1+1\nQ\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    return exit_code == 0 && output.str() == "2\n" &&
           error.str() == "error: expected number after operator\n";
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

    return EXIT_SUCCESS;
}
