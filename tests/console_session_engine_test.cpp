#include <cstdlib>
#include <deque>
#include <string>
#include <span>
#include <string_view>

#include "console_session_engine.h"
#include "console_test_utils.h"
#include "console_calc/expression_parser.h"

namespace {

using console_calc::test::definitions_equal;
using console_calc::test::expect_single_error_event;
using console_calc::test::expect_single_text_event;
using console_calc::test::expect_single_value_event;

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

console_calc::ConstantTable default_constants() {
    return {
        {"pi", 3.14159265358979323846},
        {"e", 2.71828182845904523536},
        {"tau", 6.28318530717958647692},
    };
}

bool expect_engine_basic_flow() {
    console_calc::ExpressionParser parser;
    const console_calc::ConstantTable constants = default_constants();
    console_calc::ConsoleSessionEngine engine(parser, constants);

    engine.initialize();

    const auto expression_result = engine.submit("1+1");
    if (expression_result.should_exit ||
        !expect_single_value_event("engine basic flow expression", expression_result) ||
        !std::holds_alternative<std::int64_t>(*expression_result.events[0].value) ||
        std::get<std::int64_t>(*expression_result.events[0].value) != 2 ||
        expression_result.state.stack.size() != 1 || expression_result.state.max_stack_depth != 100 ||
        engine.stack_depth() != 1) {
        return false;
    }

    const auto assignment_result = engine.submit("x:pi+1");
    if (assignment_result.should_exit || !assignment_result.events.empty() ||
        !assignment_result.state.definitions.contains("x") || !engine.definitions().contains("x")) {
        return false;
    }

    const auto variable_result = engine.submit("x");
    return !variable_result.should_exit && variable_result.events.size() == 1 &&
           variable_result.events[0].kind == console_calc::ConsoleOutputEventKind::value &&
           variable_result.events[0].value.has_value() &&
           std::holds_alternative<double>(*variable_result.events[0].value) &&
           std::get<double>(*variable_result.events[0].value) > 4.14159 &&
           std::get<double>(*variable_result.events[0].value) < 4.14160 &&
           variable_result.state.stack.size() == 2 && engine.stack_depth() == 2;
}

bool expect_engine_display_mode_and_stack_state() {
    console_calc::ExpressionParser parser;
    const console_calc::ConstantTable constants = default_constants();
    console_calc::ConsoleSessionEngine engine(parser, constants);

    engine.initialize();
    (void)engine.submit("255");

    const auto hex_result = engine.submit("hex");
    if (hex_result.should_exit || hex_result.state.display_mode !=
                                      console_calc::IntegerDisplayMode::hexadecimal) {
        return false;
    }

    const auto stack_result = engine.submit("s");
    if (!expect_single_text_event("engine list stack", stack_result, "0:0xff\n")) {
        return false;
    }

    const auto bin_result = engine.submit("bin");
    if (bin_result.should_exit ||
        bin_result.state.display_mode != console_calc::IntegerDisplayMode::binary) {
        return false;
    }

    const auto dec_result = engine.submit("dec");
    return !dec_result.should_exit &&
           dec_result.state.display_mode == console_calc::IntegerDisplayMode::decimal;
}

bool expect_engine_stack_commands() {
    console_calc::ExpressionParser parser;
    const console_calc::ConstantTable constants = default_constants();
    console_calc::ConsoleSessionEngine engine(parser, constants);

    engine.initialize();
    (void)engine.submit("1");
    (void)engine.submit("2");

    const auto dup_result = engine.submit("dup");
    if (dup_result.state.stack.size() != 3 ||
        !std::holds_alternative<std::int64_t>(dup_result.state.stack[2]) ||
        std::get<std::int64_t>(dup_result.state.stack[2]) != 2) {
        return false;
    }

    const auto swap_result = engine.submit("swap");
    if (swap_result.state.stack.size() != 3 ||
        !std::holds_alternative<std::int64_t>(swap_result.state.stack[1]) ||
        !std::holds_alternative<std::int64_t>(swap_result.state.stack[2]) ||
        std::get<std::int64_t>(swap_result.state.stack[1]) != 2 ||
        std::get<std::int64_t>(swap_result.state.stack[2]) != 2) {
        return false;
    }

    const auto drop_result = engine.submit("drop");
    if (drop_result.state.stack.size() != 2) {
        return false;
    }

    const auto clear_result = engine.submit("clear");
    return clear_result.events.empty() && clear_result.state.stack.empty() &&
           clear_result.state.max_stack_depth == 100;
}

bool expect_engine_listing_text_events() {
    console_calc::ExpressionParser parser;
    const console_calc::ConstantTable constants = default_constants();
    console_calc::ConsoleSessionEngine engine(parser, constants);

    engine.initialize();
    (void)engine.submit("x:pi+1");

    const auto vars_result = engine.submit("vars");
    if (!expect_single_text_event("engine vars", vars_result, "x:pi+1\n")) {
        return false;
    }

    const auto consts_result = engine.submit("consts");
    if (!expect_single_text_event(
            "engine consts", consts_result,
            "e:2.7182818284590451\npi:3.1415926535897931\ntau:6.2831853071795862\n")) {
        return false;
    }

    const auto funcs_result = engine.submit("funcs");
    return expect_single_text_event(
        "engine funcs", funcs_result,
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
        "  repeat/2    repeat value count times\n");
}

bool expect_engine_currency_refresh() {
    console_calc::ExpressionParser parser;
    const console_calc::ConstantTable constants = default_constants();
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
    console_calc::ConsoleSessionEngine engine(parser, constants, &provider,
                                              std::chrono::milliseconds{50}, true);

    engine.initialize();
    const auto result = engine.submit("nok2usd");
    return !result.should_exit &&
           expect_single_value_event("engine currency refresh", result) &&
           std::holds_alternative<double>(*result.events[0].value) &&
           std::get<double>(*result.events[0].value) == 0.1 &&
           result.state.definitions.contains("nok2usd");
}

bool expect_engine_currency_refresh_command_and_failure() {
    console_calc::ExpressionParser parser;
    const console_calc::ConstantTable constants = default_constants();
    FakeCurrencyRateProvider provider({{
                                         .rates = console_calc::CurrencyRateTable{
                                             {"usd", 0.2},
                                             {"cny", 0.7},
                                             {"eur", 0.09},
                                             {"gbp", 0.08},
                                             {"sek", 1.1},
                                             {"dkk", 0.65},
                                         },
                                     },
                                     {
                                         .error = "timeout",
                                     }});
    console_calc::ConsoleSessionEngine engine(parser, constants, &provider,
                                              std::chrono::milliseconds{50}, false);

    engine.initialize();
    const auto refresh_result = engine.submit("fx_refresh");
    if (refresh_result.should_exit || !refresh_result.state.definitions.contains("nok2usd")) {
        return false;
    }

    const auto first_lookup = engine.submit("nok2usd");
    if (!expect_single_value_event("engine currency lookup", first_lookup) ||
        !std::holds_alternative<double>(*first_lookup.events[0].value) ||
        std::get<double>(*first_lookup.events[0].value) != 0.2) {
        return false;
    }

    const auto failed_refresh = engine.submit("fx_refresh");
    return expect_single_error_event("engine failed currency refresh", failed_refresh,
                                     "currency refresh failed: timeout");
}

bool expect_engine_empty_command_noop() {
    console_calc::ExpressionParser parser;
    const console_calc::ConstantTable constants = default_constants();
    console_calc::ConsoleSessionEngine engine(parser, constants);

    engine.initialize();
    (void)engine.submit("1");
    const auto before = engine.state();
    const auto result = engine.submit("   ");

    return !result.should_exit && result.events.empty() &&
           result.state.stack == before.stack &&
           result.state.max_stack_depth == before.max_stack_depth &&
           result.state.display_mode == before.display_mode &&
           definitions_equal(result.state.definitions, before.definitions);
}

bool expect_engine_state_snapshot_consistency() {
    console_calc::ExpressionParser parser;
    const console_calc::ConstantTable constants = default_constants();
    console_calc::ConsoleSessionEngine engine(parser, constants);

    engine.initialize();
    (void)engine.submit("1");
    (void)engine.submit("x:pi+1");
    const auto result = engine.submit("hex");
    const auto engine_state = engine.state();

    return result.state.stack == engine_state.stack &&
           result.state.max_stack_depth == engine_state.max_stack_depth &&
           definitions_equal(result.state.definitions, engine_state.definitions) &&
           result.state.display_mode == engine_state.display_mode;
}

bool expect_engine_error_reporting() {
    console_calc::ExpressionParser parser;
    const console_calc::ConstantTable constants = default_constants();
    console_calc::ConsoleSessionEngine engine(parser, constants);

    const auto result = engine.submit("unknown_name");
    return !result.should_exit &&
           expect_single_error_event("engine error reporting", result,
                                     "unknown identifier: unknown_name");
}

}  // namespace

int main() {
    if (!expect_engine_basic_flow()) {
        return EXIT_FAILURE;
    }
    if (!expect_engine_display_mode_and_stack_state()) {
        return EXIT_FAILURE;
    }
    if (!expect_engine_stack_commands()) {
        return EXIT_FAILURE;
    }
    if (!expect_engine_listing_text_events()) {
        return EXIT_FAILURE;
    }
    if (!expect_engine_currency_refresh()) {
        return EXIT_FAILURE;
    }
    if (!expect_engine_currency_refresh_command_and_failure()) {
        return EXIT_FAILURE;
    }
    if (!expect_engine_empty_command_noop()) {
        return EXIT_FAILURE;
    }
    if (!expect_engine_state_snapshot_consistency()) {
        return EXIT_FAILURE;
    }
    if (!expect_engine_error_reporting()) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
