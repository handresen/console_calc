#pragma once

#include <chrono>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "console_command.h"
#include "console_history.h"
#include "console_line_editor.h"
#include "console_session_engine.h"
#include "console_calc/value.h"
#include "console_calc/value_format.h"

namespace console_calc {

class ExpressionParser;

class ConsoleSession {
public:
    ConsoleSession(const ExpressionParser& parser, const ConstantTable& constants,
                   std::istream& input, std::ostream& output, std::ostream& error);
    ConsoleSession(const ExpressionParser& parser, const ConstantTable& constants,
                   std::istream& input, std::ostream& output, std::ostream& error,
                   CurrencyRateProvider* currency_rate_provider,
                   std::chrono::milliseconds currency_rate_timeout,
                   bool auto_refresh_currency_rates);

    int run();

private:
    int handle_line(std::string_view line);
    [[nodiscard]] std::string prompt_text() const;

    std::istream& input_;
    std::ostream& output_;
    std::ostream& error_;
    ConsoleSessionEngine engine_;
    ConsoleHistory history_;
    ConsoleLineEditor line_editor_;
};

}  // namespace console_calc
