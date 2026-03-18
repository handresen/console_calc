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
#include "currency_rate_provider.h"
#include "expression_environment.h"
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
    void print_result(const Value& value);
    [[nodiscard]] bool try_handle_hidden_command(std::string_view line);
    void assign_definition(std::string_view name, std::string_view expression,
                           const std::optional<Value>& result_reference);
    void refresh_currency_rates(bool report_errors);
    void push_result(Value result);
    void set_stack_depth(std::size_t depth);
    [[nodiscard]] Value apply_stack_operator(char op);
    std::optional<Value> top_result() const;

    const ExpressionParser& parser_;
    const ConstantTable& constants_;
    std::istream& input_;
    std::ostream& output_;
    std::ostream& error_;
    std::vector<Value> result_stack_;
    std::size_t max_stack_depth_ = 4;
    DefinitionTable definitions_;
    IntegerDisplayMode display_mode_ = IntegerDisplayMode::decimal;
    CurrencyRateProvider* currency_rate_provider_ = nullptr;
    std::chrono::milliseconds currency_rate_timeout_{1500};
    bool auto_refresh_currency_rates_ = false;
    ConsoleHistory history_;
    ConsoleLineEditor line_editor_;
};

}  // namespace console_calc
