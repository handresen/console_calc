#include "console_calc/console_binding_facade.h"

#include <string>
#include <utility>

#include "console_listing.h"
#include "console_session_engine.h"
#include "console_calc/builtin_function.h"
#include "console_calc/expression_parser.h"

namespace console_calc {

struct ConsoleBindingFacade::Impl {
    ConstantTable constants;
    ConsoleSessionEngine engine;

    Impl(const ExpressionParser& parser, const ConstantTable& constants_table,
         CurrencyRateProvider* currency_rate_provider,
         std::chrono::milliseconds currency_rate_timeout, bool auto_refresh_currency_rates)
        : constants(constants_table),
          engine(parser, constants, currency_rate_provider, currency_rate_timeout,
                 auto_refresh_currency_rates) {}
};

namespace {

std::vector<double> to_binding_list_values(const Value& value) {
    if (!std::holds_alternative<ListValue>(value)) {
        return {};
    }

    std::vector<double> output;
    const auto& list = std::get<ListValue>(value);
    output.reserve(list.size());
    for (const auto& scalar : list) {
        output.push_back(std::visit(
            [](const auto raw_value) { return static_cast<double>(raw_value); }, scalar));
    }
    return output;
}

std::optional<BindingPositionEntry> to_binding_position(const Value& value) {
    if (const auto* position = std::get_if<PositionValue>(&value)) {
        return BindingPositionEntry{
            .latitude_deg = position->latitude_deg,
            .longitude_deg = position->longitude_deg,
        };
    }

    return std::nullopt;
}

std::string display_mode_name(IntegerDisplayMode mode) {
    switch (mode) {
    case IntegerDisplayMode::decimal:
        return "dec";
    case IntegerDisplayMode::hexadecimal:
        return "hex";
    case IntegerDisplayMode::binary:
        return "bin";
    }

    return "dec";
}

std::string function_category_name(BuiltinFunctionCategory category) {
    switch (category) {
    case BuiltinFunctionCategory::scalar:
        return "scalar";
    case BuiltinFunctionCategory::position:
        return "position";
    case BuiltinFunctionCategory::list:
        return "list";
    case BuiltinFunctionCategory::list_generation:
        return "list_generation";
    }

    return "scalar";
}

BindingStackEntry to_binding_entry(const StackEntryView& entry, IntegerDisplayMode mode) {
    return BindingStackEntry{
        .level = entry.level,
        .display = format_console_value(entry.value, mode),
        .list_values = to_binding_list_values(entry.value),
        .position = to_binding_position(entry.value),
    };
}

std::vector<BindingStackEntry> to_binding_stack(std::span<const StackEntryView> entries,
                                                IntegerDisplayMode mode) {
    std::vector<BindingStackEntry> output;
    output.reserve(entries.size());
    for (const auto& entry : entries) {
        output.push_back(to_binding_entry(entry, mode));
    }
    return output;
}

std::vector<BindingDefinitionEntry> to_binding_definitions(
    std::span<const DefinitionView> definitions) {
    std::vector<BindingDefinitionEntry> output;
    output.reserve(definitions.size());
    for (const auto& definition : definitions) {
        output.push_back(BindingDefinitionEntry{
            .name = definition.name,
            .expression = definition.expression,
        });
    }
    return output;
}

std::vector<BindingConstantEntry> to_binding_constants(std::span<const ConstantView> constants) {
    std::vector<BindingConstantEntry> output;
    output.reserve(constants.size());
    for (const auto& constant : constants) {
        output.push_back(BindingConstantEntry{
            .name = constant.name,
            .value = format_scalar(constant.value),
        });
    }
    return output;
}

std::vector<BindingFunctionEntry> to_binding_functions(std::span<const FunctionView> functions) {
    std::vector<BindingFunctionEntry> output;
    output.reserve(functions.size());
    for (const auto& function : functions) {
        output.push_back(BindingFunctionEntry{
            .name = function.name,
            .signature = function.signature,
            .category = function_category_name(function.category),
            .summary = function.summary,
        });
    }
    return output;
}

BindingSnapshot to_binding_snapshot(const ConsoleSessionSnapshot& snapshot) {
    return BindingSnapshot{
        .stack = to_binding_stack(snapshot.stack_entries, snapshot.display_mode),
        .max_stack_depth = snapshot.max_stack_depth,
        .definitions = to_binding_definitions(snapshot.definitions),
        .constants = to_binding_constants(snapshot.constants),
        .functions = to_binding_functions(snapshot.functions),
        .display_mode = display_mode_name(snapshot.display_mode),
    };
}

BindingEvent to_binding_event(const ConsoleCommandEvent& event, IntegerDisplayMode mode) {
    BindingEvent binding_event{};
    switch (event.kind) {
    case ConsoleCommandEventKind::value:
        binding_event.kind = BindingEventKind::value;
        binding_event.text = format_console_value(*event.value, mode);
        break;
    case ConsoleCommandEventKind::text:
        binding_event.kind = BindingEventKind::text;
        binding_event.text = event.text;
        break;
    case ConsoleCommandEventKind::stack_listing:
        binding_event.kind = BindingEventKind::stack_listing;
        binding_event.stack = to_binding_stack(event.stack_entries, mode);
        break;
    case ConsoleCommandEventKind::definition_listing:
        binding_event.kind = BindingEventKind::definition_listing;
        binding_event.definitions = to_binding_definitions(event.definitions);
        break;
    case ConsoleCommandEventKind::constant_listing:
        binding_event.kind = BindingEventKind::constant_listing;
        binding_event.constants = to_binding_constants(event.constants);
        break;
    case ConsoleCommandEventKind::function_listing:
        binding_event.kind = BindingEventKind::function_listing;
        binding_event.functions = to_binding_functions(event.functions);
        break;
    case ConsoleCommandEventKind::error:
        binding_event.kind = BindingEventKind::error;
        binding_event.text = event.text;
        binding_event.error = event.error;
        break;
    }

    return binding_event;
}

}  // namespace

ConsoleBindingFacade::ConsoleBindingFacade(const ExpressionParser& parser,
                                           const ConstantTable& constants,
                                           CurrencyRateProvider* currency_rate_provider,
                                           std::chrono::milliseconds currency_rate_timeout,
                                           bool auto_refresh_currency_rates)
    : impl_(std::make_unique<Impl>(parser, constants, currency_rate_provider,
                                   currency_rate_timeout, auto_refresh_currency_rates)) {}

ConsoleBindingFacade::~ConsoleBindingFacade() = default;

void ConsoleBindingFacade::initialize() { impl_->engine.initialize(); }

BindingCommandResult ConsoleBindingFacade::submit(std::string_view input) {
    const ConsoleCommandResult result = impl_->engine.submit(input);

    BindingCommandResult binding_result{
        .should_exit = result.should_exit,
        .snapshot = to_binding_snapshot(result.state),
    };
    binding_result.events.reserve(result.events.size());
    for (const auto& event : result.events) {
        binding_result.events.push_back(to_binding_event(event, result.state.display_mode));
    }
    return binding_result;
}

BindingSnapshot ConsoleBindingFacade::snapshot() const {
    return to_binding_snapshot(impl_->engine.state());
}

}  // namespace console_calc
