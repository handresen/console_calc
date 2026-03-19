#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

#include "currency_rate_provider.h"
#include "console_session_engine.h"

namespace console_calc {

class ExpressionParser;

struct BindingStackEntry {
    std::size_t level = 0;
    std::string display;
};

struct BindingDefinitionEntry {
    std::string name;
    std::string expression;
};

struct BindingConstantEntry {
    std::string name;
    std::string value;
};

struct BindingFunctionEntry {
    std::string name;
    std::string arity_label;
    std::string category;
    std::string summary;
};

struct BindingSnapshot {
    std::vector<BindingStackEntry> stack;
    std::size_t max_stack_depth = 100;
    std::vector<BindingDefinitionEntry> definitions;
    std::vector<BindingConstantEntry> constants;
    std::vector<BindingFunctionEntry> functions;
    std::string display_mode;
};

enum class BindingEventKind {
    value,
    text,
    stack_listing,
    definition_listing,
    constant_listing,
    function_listing,
    error,
};

struct BindingEvent {
    BindingEventKind kind = BindingEventKind::text;
    std::string text;
    std::vector<BindingStackEntry> stack;
    std::vector<BindingDefinitionEntry> definitions;
    std::vector<BindingConstantEntry> constants;
    std::vector<BindingFunctionEntry> functions;
};

struct BindingCommandResult {
    bool should_exit = false;
    std::vector<BindingEvent> events;
    BindingSnapshot snapshot;
};

class ConsoleBindingFacade {
public:
    ConsoleBindingFacade(const ExpressionParser& parser, const ConstantTable& constants,
                         CurrencyRateProvider* currency_rate_provider = nullptr,
                         std::chrono::milliseconds currency_rate_timeout =
                             std::chrono::milliseconds{1500},
                         bool auto_refresh_currency_rates = false);

    void initialize();
    [[nodiscard]] BindingCommandResult submit(std::string_view input);
    [[nodiscard]] BindingSnapshot snapshot() const;

private:
    ConstantTable constants_;
    ConsoleSessionEngine engine_;
};

}  // namespace console_calc
