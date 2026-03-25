#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "console_command.h"
#include "console_assignment.h"
#include "console_listing.h"
#include "console_calc/error_info.h"
#include "currency_rate_provider.h"
#include "expression_environment.h"
#include "console_calc/value.h"

namespace console_calc {

class ExpressionParser;

struct ConsoleSessionSnapshot {
    std::vector<StackEntryView> stack_entries;
    std::size_t max_stack_depth = 100;
    std::vector<DefinitionView> definitions;
    std::vector<ConstantView> constants;
    std::vector<FunctionView> functions;
    IntegerDisplayMode display_mode = IntegerDisplayMode::decimal;
};

enum class ConsoleCommandEventKind {
    value,
    text,
    stack_listing,
    definition_listing,
    constant_listing,
    function_listing,
    error,
};

struct ConsoleCommandEvent {
    ConsoleCommandEventKind kind = ConsoleCommandEventKind::text;
    std::optional<Value> value;
    std::string text;
    std::optional<ErrorInfo> error;
    std::vector<StackEntryView> stack_entries;
    std::vector<DefinitionView> definitions;
    std::vector<ConstantView> constants;
    std::vector<FunctionView> functions;
};

struct ConsoleCommandResult {
    bool should_exit = false;
    std::vector<ConsoleCommandEvent> events;
    ConsoleSessionSnapshot state;
};

class ConsoleSessionEngine {
public:
    ConsoleSessionEngine(const ExpressionParser& parser, const ConstantTable& constants,
                         CurrencyRateProvider* currency_rate_provider = nullptr,
                         std::chrono::milliseconds currency_rate_timeout =
                             std::chrono::milliseconds{1500},
                         bool auto_refresh_currency_rates = false);

    void initialize();
    [[nodiscard]] ConsoleCommandResult submit(std::string_view line);
    [[nodiscard]] ConsoleSessionSnapshot state() const;

    [[nodiscard]] std::size_t stack_depth() const;
    [[nodiscard]] IntegerDisplayMode display_mode() const;
    [[nodiscard]] std::span<const Value> stack() const;
    [[nodiscard]] const DefinitionTable& definitions() const;
    [[nodiscard]] const ConstantTable& constants() const;

private:
    [[nodiscard]] std::optional<Value> assign_definition(
        const UserAssignment& assignment, const std::optional<Value>& result_reference);
    void refresh_currency_rates(bool report_errors, ConsoleCommandResult& result);
    void push_result(Value result);
    [[nodiscard]] Value apply_stack_operator(char op);
    std::optional<Value> top_result() const;
    [[nodiscard]] ConsoleCommandResult make_result(bool should_exit = false) const;

    const ExpressionParser& parser_;
    const ConstantTable& constants_;
    std::vector<Value> result_stack_;
    std::size_t max_stack_depth_ = 100;
    DefinitionTable definitions_;
    IntegerDisplayMode display_mode_ = IntegerDisplayMode::decimal;
    CurrencyRateProvider* currency_rate_provider_ = nullptr;
    std::chrono::milliseconds currency_rate_timeout_{1500};
    bool auto_refresh_currency_rates_ = false;
    bool initialized_ = false;
};

}  // namespace console_calc
