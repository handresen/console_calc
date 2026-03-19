#include <cstdlib>
#include <deque>
#include <sstream>
#include <string_view>
#include <vector>

#include "console_calc_app.h"
#include "currency_rate_provider.h"
#include "console_test_utils.h"

namespace {

using console_calc::test::expect_console_transcript;
using console_calc::test::prompt;

class FakeCurrencyRateProvider final : public console_calc::CurrencyRateProvider {
public:
    explicit FakeCurrencyRateProvider(std::deque<console_calc::CurrencyFetchResult> responses)
        : responses_(std::move(responses)) {}

    console_calc::CurrencyFetchResult fetch_nok_rates(
        std::span<const std::string_view>, std::chrono::milliseconds) override {
        if (responses_.empty()) {
            return {.error = "no fake response configured"};
        }

        auto response = std::move(responses_.front());
        responses_.pop_front();
        return response;
    }

private:
    std::deque<console_calc::CurrencyFetchResult> responses_;
};

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
        prompt(0) +
        prompt(0) +
        prompt(0) + "-0.841471\n" +
        prompt(1) +
        prompt(1) + "0\n" +
        prompt(2);
    return expect_console_transcript("console mode late bound variables", exit_code, 0,
                                     output.str(), expected_output, error.str(), "");
}

bool expect_console_mode_list_variables() {
    const std::vector<std::string_view> args;
    std::istringstream input("vals:1,2,3,4,5,7.0,sin(pi+0.1)\nsum(vals)\nvals:4,5\nsum(vals)\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) +
        prompt(0) + "21.9002\n" +
        prompt(1) +
        prompt(1) + "9\n" +
        prompt(2);
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
        prompt(0) +
        prompt(0) +
        prompt(0) + "6\n" +
        prompt(1) +
        prompt(1) + "9\n" +
        prompt(2);
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
        prompt(0) + "{1, 2, 3}\n" +
        prompt(1) + "0:{1, 2, 3}\n" +
        prompt(1);
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
        prompt(0) + "{1, 2, 3}\n" +
        prompt(1) + "{1, 2, 3}\n" +
        prompt(2) + "6\n" +
        prompt(3);
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
        prompt(0) + "{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, <hiding 2 entries>}\n" +
        prompt(1);
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

bool expect_console_mode_map_builtin_identifier() {
    const std::vector<std::string_view> args;
    std::istringstream input("l:{1,2,3}\nmap(l,sin)\nsum(map({1,2,3},sin))\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) +
        prompt(0) + "{0.8414709848078965, 0.90929742682568171, 0.14112000805986721}\n" +
        prompt(1) + "1.89189\n" +
        prompt(2);
    return expect_console_transcript("console mode map builtin identifier", exit_code, 0,
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

bool expect_console_mode_circular_reference_error() {
    const std::vector<std::string_view> args;
    std::istringstream input("a:1\nb:a+1\na:b+1\na\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) +
        prompt(0) +
        prompt(0) +
        prompt(0) + "1\n" +
        prompt(1);
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

bool expect_console_mode_list_constants() {
    const std::vector<std::string_view> args;
    std::istringstream input("consts\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) +
        "e:2.7182818284590451\n"
        "pi:3.1415926535897931\n"
        "tau:6.2831853071795862\n" +
        prompt(0);
    return expect_console_transcript("console mode list constants", exit_code, 0, output.str(),
                                     expected_output, error.str(), "");
}

bool expect_console_mode_list_variables_and_functions() {
    const std::vector<std::string_view> args;
    std::istringstream input(
        "x:pi+1\nvals:1,2,3\nsx:sin(x)\ntotal:sum(vals)\nvars\nfuncs\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string expected_output =
        prompt(0) +
        prompt(0) +
        prompt(0) +
        prompt(0) +
        prompt(0) + "sx:sin(x)\ntotal:sum(vals)\nvals:{1, 2, 3}\nx:pi+1\n" +
        prompt(0) +
        "Scalar functions\n"
        "  abs/1       absolute value\n"
        "  cos/1       cosine in radians\n"
        "  cosd/1      cosine in degrees\n"
        "  pow/2       power\n"
        "  sin/1       sine in radians\n"
        "  sind/1      sine in degrees\n"
        "  sqrt/1      square root\n"
        "  tan/1       tangent in radians\n"
        "  tand/1      tangent in degrees\n"
        "\n"
        "List functions\n"
        "  avg/1       average of list elements\n"
        "  drop/2      drop first n list elements\n"
        "  first/2     first n list elements\n"
        "  len/1       list length\n"
        "  list_add/2  add matching list elements\n"
        "  list_div/2  divide matching list elements\n"
        "  list_mul/2  multiply matching list elements\n"
        "  list_sub/2  subtract matching list elements\n"
        "  map/2       map unary scalar builtin over list\n"
        "  max/1       maximum list element\n"
        "  min/1       minimum list element\n"
        "  product/1   product of list elements\n"
        "  reduce/2    reduce list with binary operator\n"
        "  sum/1       sum list elements\n"
        "\n"
        "List generation functions\n"
        "  geom/2-3    generate geometric series from start\n"
        "  linspace/3  generate evenly spaced values over interval\n"
        "  powers/2-3  generate successive integer powers\n"
        "  range/2-3   generate linear series from start\n"
        "  repeat/2    repeat value count times\n" +
        prompt(0);
    return expect_console_transcript("console mode list variables and functions", exit_code, 0,
                                     output.str(), expected_output, error.str(), "");
}

bool expect_console_mode_currency_refresh_on_launch() {
    const std::vector<std::string_view> args;
    FakeCurrencyRateProvider provider({{
        .rates = console_calc::CurrencyRateTable{
            {"usd", 0.1},
            {"cny", 0.7},
            {"eur", 0.09},
            {"gbp", 0.08},
            {"sek", 1.1},
            {"dkk", 0.65},
        },
    }});
    std::istringstream input("nok2usd\nusd2nok\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code =
        console_calc::run_console_calc(args, input, output, error,
                                       console_calc::ConsoleCalcOptions{
                                           .currency_rate_provider = &provider,
                                           .auto_refresh_currency_rates = true,
                                           .currency_rate_timeout = std::chrono::milliseconds{50},
                                       });
    const std::string expected_output =
        prompt(0) + "0.1\n" +
        prompt(1) + "10\n" +
        prompt(2);
    return expect_console_transcript("console mode currency refresh on launch", exit_code, 0,
                                     output.str(), expected_output, error.str(), "");
}

bool expect_console_mode_currency_refresh_command() {
    const std::vector<std::string_view> args;
    FakeCurrencyRateProvider provider({{
        .rates = console_calc::CurrencyRateTable{
            {"usd", 0.2},
            {"cny", 0.71},
            {"eur", 0.1},
            {"gbp", 0.09},
            {"sek", 1.12},
            {"dkk", 0.66},
        },
    }});
    std::istringstream input("nok2usd\nfx_refresh\nnok2usd\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code =
        console_calc::run_console_calc(args, input, output, error,
                                       console_calc::ConsoleCalcOptions{
                                           .currency_rate_provider = &provider,
                                           .auto_refresh_currency_rates = false,
                                           .currency_rate_timeout = std::chrono::milliseconds{50},
                                       });
    const std::string expected_output =
        prompt(0) + prompt(0) + prompt(0) + "0.2\n" + prompt(1);
    return expect_console_transcript("console mode currency refresh command", exit_code, 0,
                                     output.str(), expected_output, error.str(),
                                     "error: unknown identifier: nok2usd\n");
}

bool expect_console_mode_currency_refresh_offline() {
    const std::vector<std::string_view> args;
    FakeCurrencyRateProvider provider({{
                                         .error = "timeout",
                                     },
                                     {
                                         .error = "timeout",
                                     }});
    std::istringstream input("nok2usd\nfx_refresh\nq\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code =
        console_calc::run_console_calc(args, input, output, error,
                                       console_calc::ConsoleCalcOptions{
                                           .currency_rate_provider = &provider,
                                           .auto_refresh_currency_rates = true,
                                           .currency_rate_timeout = std::chrono::milliseconds{50},
                                       });
    const std::string expected_output = prompt(0) + prompt(0) + prompt(0);
    return expect_console_transcript("console mode currency refresh offline", exit_code, 0,
                                     output.str(), expected_output, error.str(),
                                     "error: unknown identifier: nok2usd\n"
                                     "error: currency refresh failed: timeout\n");
}

}  // namespace

int main() {
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
    if (!expect_console_mode_variables()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_late_bound_variables()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_list_variables()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_late_bound_list_variables()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_list_stack_values()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_list_stack_operator_error()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_list_result_reference()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_long_list_result_output()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_long_list_stack_output()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_map_builtin_identifier()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_integer_display_modes()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_radix_literals()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_circular_reference_error()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_self_circular_reference_error()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_variable_constant_conflict()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_result_reference_error()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_unknown_identifier_error()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_list_constants()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_list_variables_and_functions()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_currency_refresh_on_launch()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_currency_refresh_command()) {
        return EXIT_FAILURE;
    }
    if (!expect_console_mode_currency_refresh_offline()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
