#include "console_session_engine.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <string>
#include <string_view>

#include "console_assignment.h"
#include "console_command_executor.h"
#include "console_listing.h"
#include "currency_catalog.h"
#include "currency_definition_materializer.h"
#include "console_calc/error_info.h"
#include "console_calc/expression_parser.h"
#include "console_calc/scalar_value.h"
#include "console_calc/special_form.h"
#include "scalar_math.h"

namespace console_calc {

namespace {

[[nodiscard]] std::string trim(std::string_view text) {
    std::size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) {
        ++begin;
    }

    std::size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }

    return std::string(text.substr(begin, end - begin));
}

[[nodiscard]] bool is_listing_command(ConsoleCommandKind command) {
    return command == ConsoleCommandKind::list_stack ||
           command == ConsoleCommandKind::list_variables ||
           command == ConsoleCommandKind::list_constants ||
           command == ConsoleCommandKind::list_functions;
}

}  // namespace

ConsoleSessionEngine::ConsoleSessionEngine(const ExpressionParser& parser,
                                           const ConstantTable& constants,
                                           CurrencyRateProvider* currency_rate_provider,
                                           std::chrono::milliseconds currency_rate_timeout,
                                           bool auto_refresh_currency_rates)
    : parser_(parser),
      constants_(constants),
      currency_rate_provider_(currency_rate_provider),
      currency_rate_timeout_(currency_rate_timeout),
      auto_refresh_currency_rates_(auto_refresh_currency_rates) {}

void ConsoleSessionEngine::initialize() {
    if (initialized_) {
        return;
    }
    initialized_ = true;

    if (auto_refresh_currency_rates_) {
        ConsoleCommandResult ignored = make_result();
        refresh_currency_rates(false, ignored);
    }
}

ConsoleCommandResult ConsoleSessionEngine::submit(std::string_view line) {
    initialize();

    ConsoleCommandResult result = make_result();
    const std::string trimmed = trim(line);
    if (trimmed.empty()) {
        return result;
    }

    try {
        const ConsoleCommand command = classify_console_command(trimmed);
        if (command.kind == ConsoleCommandKind::quit) {
            result = make_result(true);
            return result;
        }

        if (command.kind == ConsoleCommandKind::refresh_currency_rates) {
            refresh_currency_rates(true, result);
            result.state = state();
            return result;
        }

        if (is_listing_command(command.kind)) {
            ConsoleCommandEvent event;
            switch (command.kind) {
            case ConsoleCommandKind::list_stack:
                event.kind = ConsoleCommandEventKind::stack_listing;
                event.stack_entries = stack_entry_views(result_stack_);
                break;
            case ConsoleCommandKind::list_variables:
                event.kind = ConsoleCommandEventKind::definition_listing;
                event.definitions = definition_views(definitions_);
                break;
            case ConsoleCommandKind::list_constants:
                event.kind = ConsoleCommandEventKind::constant_listing;
                event.constants = constant_views(constants_);
                break;
            case ConsoleCommandKind::list_functions:
                event.kind = ConsoleCommandEventKind::function_listing;
                event.functions = function_views(builtin_functions(), special_forms());
                break;
            default:
                break;
            }
            result.events.push_back(std::move(event));
            result.state = state();
            return result;
        }

        if (is_non_evaluating_console_command(command.kind)) {
            std::vector<std::string> ignored_output_lines;
            StringConsoleCommandExecutionContext context{
                .result_stack = result_stack_,
                .max_stack_depth = max_stack_depth_,
                .definitions = definitions_,
                .constants = constants_,
                .display_mode = display_mode_,
                .output_lines = ignored_output_lines,
                .mutable_stack = result_stack_,
            };
            execute_non_evaluating_console_command(command.kind, context);
            result.state = state();
            return result;
        }

        if (command.kind == ConsoleCommandKind::stack_operator) {
            const Value stack_result = apply_stack_operator(command.stack_operator);
            result.events.push_back(ConsoleCommandEvent{
                .kind = ConsoleCommandEventKind::value,
                .value = stack_result,
            });
            result.state = state();
            return result;
        }

        const std::optional<Value> result_reference = top_result();
        if (command.kind == ConsoleCommandKind::assignment) {
            const auto assignment = parse_user_assignment(trimmed);
            if (assignment.has_value()) {
                if (constants_.contains(assignment->name)) {
                    throw std::invalid_argument("cannot redefine constant: " + assignment->name);
                }
                const std::optional<Value> assigned_value =
                    assign_definition(*assignment, result_reference);
                if (assigned_value.has_value()) {
                    push_result(*assigned_value);
                    result.events.push_back(ConsoleCommandEvent{
                        .kind = ConsoleCommandEventKind::value,
                        .value = *assigned_value,
                    });
                }
                result.state = state();
                return result;
            }
        }

        const Value evaluation_result = evaluate_expanded_expression(
            parser_, trimmed, constants_, definitions_, result_reference);
        push_result(evaluation_result);
        result.events.push_back(ConsoleCommandEvent{
            .kind = ConsoleCommandEventKind::value,
            .value = evaluation_result,
        });
    } catch (const std::exception& ex) {
        const ErrorInfo error = infer_error_info(ex.what());
        result.events.push_back(ConsoleCommandEvent{
            .kind = ConsoleCommandEventKind::error,
            .text = error.message,
            .error = error,
        });
    }

    result.state = state();
    return result;
}

ConsoleSessionSnapshot ConsoleSessionEngine::state() const {
    return ConsoleSessionSnapshot{
        .stack_entries = stack_entry_views(result_stack_),
        .max_stack_depth = max_stack_depth_,
        .definitions = definition_views(definitions_),
        .constants = constant_views(constants_),
        .functions = function_views(builtin_functions(), special_forms()),
        .display_mode = display_mode_,
    };
}

std::size_t ConsoleSessionEngine::stack_depth() const { return result_stack_.size(); }

IntegerDisplayMode ConsoleSessionEngine::display_mode() const { return display_mode_; }

std::span<const Value> ConsoleSessionEngine::stack() const { return result_stack_; }

const DefinitionTable& ConsoleSessionEngine::definitions() const { return definitions_; }

const ConstantTable& ConsoleSessionEngine::constants() const { return constants_; }

ConsoleCommandResult ConsoleSessionEngine::make_result(bool should_exit) const {
    return ConsoleCommandResult{
        .should_exit = should_exit,
        .state = state(),
    };
}

std::optional<Value> ConsoleSessionEngine::assign_definition(
    const UserAssignment& assignment, const std::optional<Value>& result_reference) {
    const std::string normalized_expression =
        normalize_assignment_expression(assignment.expression);
    if (assignment.is_function()) {
        if (assignment.emit_result) {
            throw std::invalid_argument("'#' is only supported for value assignments");
        }
        definitions_[assignment.name] = make_function_definition(assignment.parameters,
                                                                 normalized_expression);
        return std::nullopt;
    }

    DefinitionTable validation_definitions = definitions_;
    validation_definitions[assignment.name] = make_value_definition(normalized_expression);
    const std::string expanded_expression = expand_expression_identifiers(
        normalized_expression, constants_, validation_definitions, result_reference);
    (void)parser_.evaluate_value(expanded_expression);
    definitions_[assignment.name] = make_value_definition(normalized_expression);
    if (!assignment.emit_result) {
        return std::nullopt;
    }

    return evaluate_expanded_expression(parser_, assignment.name, constants_, definitions_,
                                        result_reference);
}

void ConsoleSessionEngine::refresh_currency_rates(bool report_errors,
                                                  ConsoleCommandResult&) {
    if (currency_rate_provider_ == nullptr) {
        if (report_errors) {
            throw std::invalid_argument("currency refresh is unavailable");
        }
        return;
    }

    std::array<std::string_view, k_console_currency_catalog.size()> requested_codes{};
    for (std::size_t index = 0; index < k_console_currency_catalog.size(); ++index) {
        requested_codes[index] = k_console_currency_catalog[index].lower_code;
    }

    const CurrencyFetchResult fetch_result =
        currency_rate_provider_->fetch_nok_rates(requested_codes, currency_rate_timeout_);
    if (!fetch_result.succeeded()) {
        if (report_errors) {
            throw std::invalid_argument("currency refresh failed: " + fetch_result.error);
        }
        return;
    }

    apply_currency_rate_definitions(definitions_, *fetch_result.rates);
}

void ConsoleSessionEngine::push_result(Value result) {
    if (result_stack_.size() >= max_stack_depth_) {
        result_stack_.erase(result_stack_.begin());
    }

    result_stack_.push_back(std::move(result));
}
Value ConsoleSessionEngine::apply_stack_operator(char op) {
    if (result_stack_.size() < 2) {
        throw std::invalid_argument("stack requires at least two values");
    }

    const Value rhs_value = result_stack_.back();
    result_stack_.pop_back();
    const Value lhs_value = result_stack_.back();
    result_stack_.pop_back();

    if (!std::holds_alternative<std::int64_t>(lhs_value) &&
        !std::holds_alternative<double>(lhs_value)) {
        result_stack_.push_back(lhs_value);
        result_stack_.push_back(rhs_value);
        throw std::invalid_argument("stack operator requires scalar values");
    }
    if (!std::holds_alternative<std::int64_t>(rhs_value) &&
        !std::holds_alternative<double>(rhs_value)) {
        result_stack_.push_back(lhs_value);
        result_stack_.push_back(rhs_value);
        throw std::invalid_argument("stack operator requires scalar values");
    }

    const ScalarValue lhs = std::holds_alternative<std::int64_t>(lhs_value)
                                ? ScalarValue{std::get<std::int64_t>(lhs_value)}
                                : ScalarValue{std::get<double>(lhs_value)};
    const ScalarValue rhs = std::holds_alternative<std::int64_t>(rhs_value)
                                ? ScalarValue{std::get<std::int64_t>(rhs_value)}
                                : ScalarValue{std::get<double>(rhs_value)};

    try {
        const BinaryOperator binary_operator = [&]() {
            switch (op) {
            case '+':
                return BinaryOperator::add;
            case '-':
                return BinaryOperator::subtract;
            case '*':
                return BinaryOperator::multiply;
            case '/':
                return BinaryOperator::divide;
            case '%':
                return BinaryOperator::modulo;
            case '^':
                return BinaryOperator::power;
            case '&':
                return BinaryOperator::bitwise_and;
            case '|':
                return BinaryOperator::bitwise_or;
            default:
                throw std::invalid_argument("unknown stack operator");
            }
        }();
        const Value result = to_value(apply_binary_operator(binary_operator, lhs, rhs));
        result_stack_.push_back(result);
        return result;
    } catch (...) {
        result_stack_.push_back(lhs_value);
        result_stack_.push_back(rhs_value);
        throw;
    }
}

std::optional<Value> ConsoleSessionEngine::top_result() const {
    if (result_stack_.empty()) {
        return std::nullopt;
    }

    return result_stack_.back();
}

}  // namespace console_calc
