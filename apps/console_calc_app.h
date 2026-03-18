#pragma once

#include <chrono>
#include <iosfwd>
#include <span>
#include <string_view>

namespace console_calc {

class CurrencyRateProvider;

struct ConsoleCalcOptions {
    CurrencyRateProvider* currency_rate_provider = nullptr;
    bool auto_refresh_currency_rates = false;
    std::chrono::milliseconds currency_rate_timeout{1500};
};

int run_console_calc(std::span<const std::string_view> args, std::istream& input,
                     std::ostream& output, std::ostream& error);
int run_console_calc(std::span<const std::string_view> args, std::istream& input,
                     std::ostream& output, std::ostream& error,
                     const ConsoleCalcOptions& options);

}  // namespace console_calc
