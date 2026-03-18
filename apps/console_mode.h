#pragma once

#include <chrono>
#include <iosfwd>
#include <string>
#include <unordered_map>

namespace console_calc {

using ConstantTable = std::unordered_map<std::string, double>;
class CurrencyRateProvider;
class ExpressionParser;

int run_console_mode(const ExpressionParser& parser, const ConstantTable& constants,
                     std::istream& input, std::ostream& output, std::ostream& error);
int run_console_mode(const ExpressionParser& parser, const ConstantTable& constants,
                     std::istream& input, std::ostream& output, std::ostream& error,
                     CurrencyRateProvider* currency_rate_provider,
                     std::chrono::milliseconds currency_rate_timeout,
                     bool auto_refresh_currency_rates);

}  // namespace console_calc
