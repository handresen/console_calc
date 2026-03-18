#include <cstdlib>
#include <string_view>

#include "currency_definition_materializer.h"
#include "currency_rate_parser.h"

namespace {

bool expect_currency_response_parsing() {
    constexpr std::string_view response =
        R"({"amount":1.0,"base":"NOK","date":"2026-03-18","rates":{"USD":0.1,"CNY":0.7,"EUR":0.09}})";
    constexpr std::string_view currencies[] = {"usd", "cny", "eur"};

    const auto result = console_calc::parse_nok_currency_rates_response(response, currencies);
    return result.succeeded() && result.rates->at("usd") == 0.1 &&
           result.rates->at("cny") == 0.7 && result.rates->at("eur") == 0.09;
}

bool expect_currency_response_parse_failure() {
    constexpr std::string_view response =
        R"({"amount":1.0,"base":"NOK","date":"2026-03-18","rates":{"USD":0.1}})";
    constexpr std::string_view currencies[] = {"usd", "cny"};

    const auto result = console_calc::parse_nok_currency_rates_response(response, currencies);
    return !result.succeeded() && result.error == "currency response did not include CNY";
}

bool expect_currency_definition_materialization() {
    console_calc::DefinitionTable definitions;
    const console_calc::CurrencyRateTable rates{
        {"usd", 0.1},
        {"eur", 0.09},
    };

    console_calc::apply_currency_rate_definitions(definitions, rates);
    return definitions.at("nok2usd").expression == "0.10000000000000001" &&
           definitions.at("usd2nok").expression == "10" &&
           definitions.at("nok2eur").expression == "0.089999999999999997" &&
           definitions.at("eur2nok").expression == "11.111111111111111";
}

}  // namespace

int main() {
    if (!expect_currency_response_parsing()) {
        return EXIT_FAILURE;
    }
    if (!expect_currency_response_parse_failure()) {
        return EXIT_FAILURE;
    }
    if (!expect_currency_definition_materialization()) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
