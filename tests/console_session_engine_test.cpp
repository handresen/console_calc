#include <cstdlib>
#include <deque>
#include <span>
#include <string_view>

#include "console_session_engine.h"
#include "console_calc/expression_parser.h"

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

bool expect_engine_basic_flow() {
    console_calc::ExpressionParser parser;
    const console_calc::ConstantTable constants{
        {"pi", 3.14159265358979323846},
        {"e", 2.71828182845904523536},
        {"tau", 6.28318530717958647692},
    };
    console_calc::ConsoleSessionEngine engine(parser, constants);

    engine.initialize();

    const auto expression_result = engine.submit("1+1");
    if (expression_result.should_exit || expression_result.emitted_values.size() != 1 ||
        !std::holds_alternative<std::int64_t>(expression_result.emitted_values[0]) ||
        std::get<std::int64_t>(expression_result.emitted_values[0]) != 2 ||
        engine.stack_depth() != 1) {
        return false;
    }

    const auto assignment_result = engine.submit("x:pi+1");
    if (assignment_result.should_exit || !assignment_result.output_lines.empty() ||
        !assignment_result.error_lines.empty() || !engine.definitions().contains("x")) {
        return false;
    }

    const auto variable_result = engine.submit("x");
    return !variable_result.should_exit && variable_result.emitted_values.size() == 1 &&
           std::holds_alternative<double>(variable_result.emitted_values[0]) &&
           std::get<double>(variable_result.emitted_values[0]) > 4.14159 &&
           std::get<double>(variable_result.emitted_values[0]) < 4.14160 &&
           engine.stack_depth() == 2;
}

bool expect_engine_currency_refresh() {
    console_calc::ExpressionParser parser;
    const console_calc::ConstantTable constants{
        {"pi", 3.14159265358979323846},
        {"e", 2.71828182845904523536},
        {"tau", 6.28318530717958647692},
    };
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
    return !result.should_exit && result.error_lines.empty() &&
           result.emitted_values.size() == 1 &&
           std::holds_alternative<double>(result.emitted_values[0]) &&
           std::get<double>(result.emitted_values[0]) == 0.1;
}

bool expect_engine_error_reporting() {
    console_calc::ExpressionParser parser;
    const console_calc::ConstantTable constants{
        {"pi", 3.14159265358979323846},
        {"e", 2.71828182845904523536},
        {"tau", 6.28318530717958647692},
    };
    console_calc::ConsoleSessionEngine engine(parser, constants);

    const auto result = engine.submit("unknown_name");
    return !result.should_exit && result.output_lines.empty() &&
           result.error_lines.size() == 1 &&
           result.error_lines[0] == "unknown identifier: unknown_name";
}

}  // namespace

int main() {
    if (!expect_engine_basic_flow()) {
        return EXIT_FAILURE;
    }
    if (!expect_engine_currency_refresh()) {
        return EXIT_FAILURE;
    }
    if (!expect_engine_error_reporting()) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
