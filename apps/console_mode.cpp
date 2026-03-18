#include "console_mode.h"

#include <chrono>

#include "currency_rate_provider.h"
#include "console_session.h"

namespace console_calc {

int run_console_mode(const ExpressionParser& parser, const ConstantTable& constants,
                     std::istream& input, std::ostream& output, std::ostream& error) {
    return run_console_mode(parser, constants, input, output, error, nullptr,
                            std::chrono::milliseconds{1500}, false);
}

int run_console_mode(const ExpressionParser& parser, const ConstantTable& constants,
                     std::istream& input, std::ostream& output, std::ostream& error,
                     CurrencyRateProvider* currency_rate_provider,
                     std::chrono::milliseconds currency_rate_timeout,
                     bool auto_refresh_currency_rates) {
    ConsoleSession session(parser, constants, input, output, error, currency_rate_provider,
                           currency_rate_timeout, auto_refresh_currency_rates);
    return session.run();
}

}  // namespace console_calc
