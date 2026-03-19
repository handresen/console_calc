#include "currency_rate_parser.h"

#include <cctype>
#include <optional>
#include <regex>
#include <string>

namespace console_calc {

namespace {

[[nodiscard]] std::optional<double> parse_rate(std::string_view body, std::string_view currency) {
    const std::regex pattern(
        "\"" + std::string(currency) + "\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?)");
    const std::string body_text(body);
    std::smatch match;
    if (!std::regex_search(body_text, match, pattern)) {
        return std::nullopt;
    }

    try {
        return std::stod(match[1].str());
    } catch (...) {
        return std::nullopt;
    }
}

[[nodiscard]] std::string uppercase_code(std::string_view code) {
    std::string upper;
    upper.reserve(code.size());
    for (const char ch : code) {
        upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    }
    return upper;
}

}  // namespace

CurrencyFetchResult parse_nok_currency_rates_response(
    std::string_view response_body, std::span<const std::string_view> currencies) {
    CurrencyRateTable rates;
    for (const std::string_view currency : currencies) {
        const std::string upper = uppercase_code(currency);
        const auto parsed_rate = parse_rate(response_body, upper);
        if (!parsed_rate.has_value()) {
            return {.error = "currency response did not include " + upper};
        }
        rates.emplace(std::string(currency), *parsed_rate);
    }

    return {.rates = std::move(rates)};
}

}  // namespace console_calc
