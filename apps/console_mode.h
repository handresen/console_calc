#pragma once

#include <chrono>
#include <iosfwd>

#include "console_calc/currency_rate_provider.h"
#include "console_calc/session_environment.h"

namespace console_calc {

class ExpressionParser;

int run_console_mode(const ExpressionParser& parser, const ConstantTable& constants,
                     std::istream& input, std::ostream& output, std::ostream& error);
int run_console_mode(const ExpressionParser& parser, const ConstantTable& constants,
                     std::istream& input, std::ostream& output, std::ostream& error,
                     CurrencyRateProvider* currency_rate_provider,
                     std::chrono::milliseconds currency_rate_timeout,
                     bool auto_refresh_currency_rates);

}  // namespace console_calc
