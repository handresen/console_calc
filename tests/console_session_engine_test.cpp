#include <cstdlib>
#include <deque>
#include <string>
#include <span>
#include <string_view>

#include "console_session_engine.h"
#include "console_test_utils.h"
#include "console_calc/expression_parser.h"

namespace {

using console_calc::test::expect_single_error_event;
using console_calc::test::expect_single_constant_listing_event;
using console_calc::test::expect_single_definition_listing_event;
using console_calc::test::expect_single_function_listing_event;
using console_calc::test::expect_single_stack_listing_event;
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

bool contains_definition(std::span<const console_calc::DefinitionView> definitions,
                         std::string_view name, std::string_view expression) {
    for (const auto& definition : definitions) {
        if (definition.name == name && definition.expression == expression) {
            return true;
        }
    }
    return false;
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
        expression_result.state.stack_entries.size() != 1 ||
        expression_result.state.stack_entries[0].level != 0 ||
        !std::holds_alternative<std::int64_t>(expression_result.state.stack_entries[0].value) ||
        std::get<std::int64_t>(expression_result.state.stack_entries[0].value) != 2 ||
        expression_result.state.max_stack_depth != 100 ||
        engine.stack_depth() != 1) {
        return false;
    }

    const auto assignment_result = engine.submit("x:pi+1");
    if (assignment_result.should_exit || !assignment_result.events.empty() ||
        !contains_definition(assignment_result.state.definitions, "x", "pi+1") ||
        !engine.definitions().contains("x")) {
        return false;
    }

    const auto variable_result = engine.submit("x");
    return !variable_result.should_exit && variable_result.events.size() == 1 &&
           variable_result.events[0].kind == console_calc::ConsoleOutputEventKind::value &&
           variable_result.events[0].value.has_value() &&
           std::holds_alternative<double>(*variable_result.events[0].value) &&
           std::get<double>(*variable_result.events[0].value) > 4.14159 &&
           std::get<double>(*variable_result.events[0].value) < 4.14160 &&
           variable_result.state.stack_entries.size() == 2 && engine.stack_depth() == 2;
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
    if (!expect_single_stack_listing_event("engine list stack", stack_result) ||
        stack_result.events[0].stack_entries.size() != 1 ||
        stack_result.events[0].stack_entries[0].level != 0 ||
        !std::holds_alternative<std::int64_t>(stack_result.events[0].stack_entries[0].value) ||
        std::get<std::int64_t>(stack_result.events[0].stack_entries[0].value) != 255) {
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
    if (dup_result.state.stack_entries.size() != 3 ||
        !std::holds_alternative<std::int64_t>(dup_result.state.stack_entries[2].value) ||
        std::get<std::int64_t>(dup_result.state.stack_entries[2].value) != 2) {
        return false;
    }

    const auto swap_result = engine.submit("swap");
    if (swap_result.state.stack_entries.size() != 3 ||
        !std::holds_alternative<std::int64_t>(swap_result.state.stack_entries[1].value) ||
        !std::holds_alternative<std::int64_t>(swap_result.state.stack_entries[2].value) ||
        std::get<std::int64_t>(swap_result.state.stack_entries[1].value) != 2 ||
        std::get<std::int64_t>(swap_result.state.stack_entries[2].value) != 2) {
        return false;
    }

    const auto drop_result = engine.submit("drop");
    if (drop_result.state.stack_entries.size() != 2) {
        return false;
    }

    const auto clear_result = engine.submit("clear");
    return clear_result.events.empty() && clear_result.state.stack_entries.empty() &&
           clear_result.state.max_stack_depth == 100;
}

bool expect_engine_listing_events() {
    console_calc::ExpressionParser parser;
    const console_calc::ConstantTable constants = default_constants();
    console_calc::ConsoleSessionEngine engine(parser, constants);

    engine.initialize();
    (void)engine.submit("x:pi+1");

    const auto vars_result = engine.submit("vars");
    if (!expect_single_definition_listing_event("engine vars", vars_result) ||
        vars_result.events[0].definitions.size() != 1 ||
        vars_result.events[0].definitions[0].name != "x" ||
        vars_result.events[0].definitions[0].expression != "pi+1") {
        return false;
    }

    const auto consts_result = engine.submit("consts");
    if (!expect_single_constant_listing_event("engine consts", consts_result) ||
        consts_result.events[0].constants.size() != 3 ||
        consts_result.events[0].constants[0].name != "e" ||
        !std::holds_alternative<double>(consts_result.events[0].constants[0].value) ||
        std::get<double>(consts_result.events[0].constants[0].value) !=
            2.7182818284590451) {
        return false;
    }

    const auto funcs_result = engine.submit("funcs");
    return expect_single_function_listing_event("engine funcs", funcs_result) &&
           !funcs_result.events[0].functions.empty() &&
           funcs_result.events[0].functions.front().name == "abs" &&
           funcs_result.events[0].functions.front().arity_label == "1" &&
           funcs_result.events[0].functions.front().category ==
               console_calc::BuiltinFunctionCategory::scalar &&
           funcs_result.events[0].functions.back().name == "repeat" &&
           funcs_result.events[0].functions.back().category ==
               console_calc::BuiltinFunctionCategory::list_generation;
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
           contains_definition(result.state.definitions, "nok2usd", "0.10000000000000001");
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
    if (refresh_result.should_exit ||
        !contains_definition(refresh_result.state.definitions, "nok2usd",
                             "0.20000000000000001")) {
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
           result.state.stack_entries.size() == before.stack_entries.size() &&
           result.state.max_stack_depth == before.max_stack_depth &&
           result.state.display_mode == before.display_mode &&
           result.state.definitions.size() == before.definitions.size() &&
           result.state.constants.size() == before.constants.size() &&
           result.state.functions.size() == before.functions.size();
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

    return result.state.stack_entries.size() == engine_state.stack_entries.size() &&
           result.state.max_stack_depth == engine_state.max_stack_depth &&
           result.state.definitions.size() == engine_state.definitions.size() &&
           result.state.constants.size() == engine_state.constants.size() &&
           result.state.functions.size() == engine_state.functions.size() &&
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
    if (!expect_engine_listing_events()) {
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
