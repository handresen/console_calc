#pragma once

#include <chrono>
#include <deque>
#include <string_view>

#include "console_calc_app.h"
#include "currency_rate_provider.h"

namespace console_calc::test {

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

bool expect_console_mode_basic_behaviors();
bool expect_console_mode_definition_behaviors();
bool expect_console_mode_list_behaviors();
bool expect_console_mode_listing_and_currency_behaviors();

}  // namespace console_calc::test
