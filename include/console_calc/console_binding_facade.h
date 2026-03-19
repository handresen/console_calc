#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "console_calc/currency_rate_provider.h"
#include "console_calc/error_info.h"
#include "console_calc/session_environment.h"
#include "console_calc/value_format.h"

namespace console_calc {

class ExpressionParser;

struct BindingPositionEntry {
    double latitude_deg = 0.0;
    double longitude_deg = 0.0;
};

struct BindingStackEntry {
    std::size_t level = 0;
    std::string display;
    std::vector<double> list_values;
    std::optional<BindingPositionEntry> position;
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
    std::string signature;
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
    std::optional<ErrorInfo> error;
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
    ~ConsoleBindingFacade();

    void initialize();
    [[nodiscard]] BindingCommandResult submit(std::string_view input);
    [[nodiscard]] BindingSnapshot snapshot() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace console_calc
