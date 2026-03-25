#include <cstdlib>
#include <deque>
#include <span>
#include <string_view>

#include "compile_time_constants.h"
#include "console_calc/console_binding_facade.h"
#include "console_calc/expression_parser.h"
#include "currency_rate_provider.h"

namespace {

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
    return console_calc::builtin_constant_table();
}

bool expect_binding_value_and_snapshot_flow() {
    console_calc::ExpressionParser parser;
    console_calc::ConsoleBindingFacade facade(parser, default_constants());

    facade.initialize();
    const auto value_result = facade.submit("1+1");
    if (value_result.should_exit || value_result.events.size() != 1 ||
        value_result.events[0].kind != console_calc::BindingEventKind::value ||
        value_result.events[0].text != "2" || value_result.snapshot.display_mode != "dec" ||
        value_result.snapshot.stack.size() != 1 || value_result.snapshot.stack[0].display != "2") {
        return false;
    }

    const auto assignment_result = facade.submit("x:pi+1");
    if (assignment_result.should_exit || !assignment_result.events.empty() ||
        assignment_result.snapshot.definitions.size() != 1 ||
        assignment_result.snapshot.definitions[0].name != "x" ||
        assignment_result.snapshot.definitions[0].expression != "pi+1") {
        return false;
    }

    const auto snapshot = facade.snapshot();
    bool has_math = false;
    bool has_conversion = false;
    bool has_physical = false;
    for (const auto& constant : snapshot.constants) {
        has_math = has_math || constant.name == "m.pi";
        has_conversion = has_conversion || constant.name == "c.deg";
        has_physical = has_physical || constant.name == "ph.c";
    }

    return snapshot.stack.size() == 1 && snapshot.stack[0].display == "2" &&
           snapshot.definitions.size() == 1 && snapshot.constants.size() >= 10 &&
           has_math && has_conversion && has_physical && !snapshot.functions.empty();
}

bool expect_binding_function_definition_snapshot() {
    console_calc::ExpressionParser parser;
    console_calc::ConsoleBindingFacade facade(parser, default_constants());

    facade.initialize();
    const auto assignment_result = facade.submit("f(x):x+1");
    return !assignment_result.should_exit && assignment_result.events.empty() &&
           assignment_result.snapshot.definitions.size() == 1 &&
           assignment_result.snapshot.definitions[0].name == "f(x)" &&
           assignment_result.snapshot.definitions[0].expression == "x+1";
}

bool expect_binding_echoed_value_assignment() {
    console_calc::ExpressionParser parser;
    console_calc::ConsoleBindingFacade facade(parser, default_constants());

    facade.initialize();
    const auto assignment_result = facade.submit("#x:pi+1");
    if (assignment_result.should_exit || assignment_result.events.size() != 1 ||
        assignment_result.events[0].kind != console_calc::BindingEventKind::value ||
        assignment_result.snapshot.definitions.size() != 1 ||
        assignment_result.snapshot.definitions[0].name != "x" ||
        assignment_result.snapshot.definitions[0].expression != "pi+1" ||
        assignment_result.snapshot.stack.size() != 1) {
        return false;
    }

    const auto function_error = facade.submit("#f(x):x+1");
    return !function_error.should_exit && function_error.events.size() == 1 &&
           function_error.events[0].kind == console_calc::BindingEventKind::error &&
           function_error.events[0].text == "'#' is only supported for value assignments";
}

bool expect_binding_multi_argument_function_snapshot() {
    console_calc::ExpressionParser parser;
    console_calc::ConsoleBindingFacade facade(parser, default_constants());

    facade.initialize();
    const auto assignment_result = facade.submit("pair_sum(x,y):x+y");
    if (assignment_result.should_exit || !assignment_result.events.empty() ||
        assignment_result.snapshot.definitions.size() != 1 ||
        assignment_result.snapshot.definitions[0].name != "pair_sum(x, y)" ||
        assignment_result.snapshot.definitions[0].expression != "x+y") {
        return false;
    }

    const auto value_result = facade.submit("pair_sum(2,5)");
    return !value_result.should_exit && value_result.events.size() == 1 &&
           value_result.events[0].kind == console_calc::BindingEventKind::value &&
           value_result.events[0].text == "7";
}

bool expect_binding_listing_and_display_modes() {
    console_calc::ExpressionParser parser;
    console_calc::ConsoleBindingFacade facade(parser, default_constants());

    facade.initialize();
    (void)facade.submit("255");
    const auto hex_result = facade.submit("hex");
    if (hex_result.snapshot.display_mode != "hex") {
        return false;
    }

    const auto stack_result = facade.submit("s");
    if (stack_result.events.size() != 1 ||
        stack_result.events[0].kind != console_calc::BindingEventKind::stack_listing ||
        stack_result.events[0].stack.size() != 1 ||
        stack_result.events[0].stack[0].display != "0xff") {
        return false;
    }

    const auto funcs_result = facade.submit("funcs");
    return funcs_result.events.size() == 1 &&
           funcs_result.events[0].kind == console_calc::BindingEventKind::function_listing &&
           !funcs_result.events[0].functions.empty() &&
           funcs_result.events[0].functions.front().category == "scalar";
}

bool expect_binding_currency_and_errors() {
    console_calc::ExpressionParser parser;
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
    console_calc::ConsoleBindingFacade facade(parser, default_constants(), &provider,
                                              std::chrono::milliseconds{50}, true);

    facade.initialize();
    const auto currency_result = facade.submit("nok2usd");
    if (currency_result.events.size() != 1 ||
        currency_result.events[0].kind != console_calc::BindingEventKind::value ||
        currency_result.events[0].text != "0.10000000000000001") {
        return false;
    }

    const auto error_result = facade.submit("unknown_name");
    return error_result.events.size() == 1 &&
           error_result.events[0].kind == console_calc::BindingEventKind::error &&
           error_result.events[0].text == "unknown identifier: unknown_name" &&
           error_result.events[0].error.has_value() &&
           error_result.events[0].error->message == "unknown identifier: unknown_name" &&
           !error_result.events[0].error->expected_signature.has_value();
}

}  // namespace

int main() {
    if (!expect_binding_value_and_snapshot_flow()) {
        return EXIT_FAILURE;
    }
    if (!expect_binding_listing_and_display_modes()) {
        return EXIT_FAILURE;
    }
    if (!expect_binding_echoed_value_assignment()) {
        return EXIT_FAILURE;
    }
    if (!expect_binding_function_definition_snapshot()) {
        return EXIT_FAILURE;
    }
    if (!expect_binding_multi_argument_function_snapshot()) {
        return EXIT_FAILURE;
    }
    if (!expect_binding_currency_and_errors()) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
