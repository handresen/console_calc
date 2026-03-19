#pragma once

#include <chrono>
#include <iosfwd>
#include <span>
#include <string_view>

#include "console_calc/currency_rate_provider.h"

namespace console_calc {

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
