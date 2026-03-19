#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "console_command.h"
#include "currency_rate_provider.h"
#include "expression_environment.h"
#include "console_calc/value.h"
#include "console_calc/value_format.h"

namespace console_calc {

class ExpressionParser;

struct ConsoleEngineCommandResult {
    bool should_exit = false;
    std::vector<Value> emitted_values;
    std::vector<std::string> output_lines;
    std::vector<std::string> error_lines;
};

class ConsoleSessionEngine {
public:
    ConsoleSessionEngine(const ExpressionParser& parser, const ConstantTable& constants,
                         CurrencyRateProvider* currency_rate_provider = nullptr,
                         std::chrono::milliseconds currency_rate_timeout =
                             std::chrono::milliseconds{1500},
                         bool auto_refresh_currency_rates = false);

    void initialize();
    [[nodiscard]] ConsoleEngineCommandResult submit(std::string_view line);

    [[nodiscard]] std::size_t stack_depth() const;
    [[nodiscard]] IntegerDisplayMode display_mode() const;
    [[nodiscard]] std::span<const Value> stack() const;
    [[nodiscard]] const DefinitionTable& definitions() const;
    [[nodiscard]] const ConstantTable& constants() const;

private:
    [[nodiscard]] bool try_handle_hidden_command(std::string_view line,
                                                 ConsoleEngineCommandResult& result);
    void assign_definition(std::string_view name, std::string_view expression,
                           const std::optional<Value>& result_reference);
    void refresh_currency_rates(bool report_errors, ConsoleEngineCommandResult& result);
    void push_result(Value result);
    void set_stack_depth(std::size_t depth);
    [[nodiscard]] Value apply_stack_operator(char op);
    std::optional<Value> top_result() const;

    const ExpressionParser& parser_;
    const ConstantTable& constants_;
    std::vector<Value> result_stack_;
    std::size_t max_stack_depth_ = 4;
    DefinitionTable definitions_;
    IntegerDisplayMode display_mode_ = IntegerDisplayMode::decimal;
    CurrencyRateProvider* currency_rate_provider_ = nullptr;
    std::chrono::milliseconds currency_rate_timeout_{1500};
    bool auto_refresh_currency_rates_ = false;
    bool initialized_ = false;
};

}  // namespace console_calc
