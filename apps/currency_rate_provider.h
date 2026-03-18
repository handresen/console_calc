#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

#include "expression_environment.h"

namespace console_calc {

using CurrencyRateTable = std::unordered_map<std::string, double>;

struct CurrencyFetchResult {
    std::optional<CurrencyRateTable> rates;
    std::string error;

    [[nodiscard]] bool succeeded() const { return rates.has_value(); }
};

class CurrencyRateProvider {
public:
    virtual ~CurrencyRateProvider() = default;

    virtual CurrencyFetchResult fetch_nok_rates(std::span<const std::string_view> currencies,
                                                std::chrono::milliseconds timeout) = 0;
};

[[nodiscard]] std::unique_ptr<CurrencyRateProvider> make_default_currency_rate_provider();

}  // namespace console_calc
