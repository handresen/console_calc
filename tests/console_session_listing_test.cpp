#include "console_session_test_support.h"

#include <sstream>
#include <string_view>
#include <vector>

#include "console_test_utils.h"

namespace console_calc::test {
namespace {

using console_calc::test::expect_console_transcript;
using console_calc::test::prompt;

bool expect_console_mode_list_constants() {
    const std::vector<std::string_view> args;
    std::istringstream input("consts\nq\n");
    std::ostringstream output;
    std::ostringstream error;
    const int exit_code = console_calc::run_console_calc(args, input, output, error);
    const std::string actual_output = output.str();
    return exit_code == 0 && error.str().empty() &&
           actual_output.starts_with(prompt(0)) &&
           actual_output.ends_with(prompt(0)) &&
           actual_output.find("[root]\n") != std::string::npos &&
           actual_output.find("[m]\n") != std::string::npos &&
           actual_output.find("[c]\n") != std::string::npos &&
           actual_output.find("[ph]\n") != std::string::npos &&
           actual_output.find("  e:2.7182818284590451\n") != std::string::npos &&
           actual_output.find("  m.pi:3.1415926535897931\n") != std::string::npos &&
           actual_output.find("  c.deg:0.017453292519943295\n") != std::string::npos &&
           actual_output.find("  ph.c:299792458\n") != std::string::npos;
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
        console_calc::format_function_listing(console_calc::builtin_functions(),
                                              console_calc::special_forms()) +
        prompt(0);
    return expect_console_transcript("console mode list variables and functions", exit_code, 0,
                                     output.str(), expected_output, error.str(), "");
}

bool expect_console_mode_currency_refresh_on_launch() {
    const std::vector<std::string_view> args;
    FakeCurrencyRateProvider provider({{
        .rates = console_calc::CurrencyRateTable{
            {"usd", 0.1}, {"cny", 0.7}, {"eur", 0.09}, {"gbp", 0.08}, {"sek", 1.1}, {"dkk", 0.65},
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
        prompt(0) + "0.1\n" + prompt(1) + "10\n" + prompt(2);
    return expect_console_transcript("console mode currency refresh on launch", exit_code, 0,
                                     output.str(), expected_output, error.str(), "");
}

bool expect_console_mode_currency_refresh_command() {
    const std::vector<std::string_view> args;
    FakeCurrencyRateProvider provider({{
        .rates = console_calc::CurrencyRateTable{
            {"usd", 0.2}, {"cny", 0.71}, {"eur", 0.1}, {"gbp", 0.09}, {"sek", 1.12}, {"dkk", 0.66},
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
    FakeCurrencyRateProvider provider({{.error = "timeout"}, {.error = "timeout"}});
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

bool expect_console_mode_listing_and_currency_behaviors() {
    return expect_console_mode_list_constants() &&
           expect_console_mode_list_variables_and_functions() &&
           expect_console_mode_currency_refresh_on_launch() &&
           expect_console_mode_currency_refresh_command() &&
           expect_console_mode_currency_refresh_offline();
}

}  // namespace console_calc::test
