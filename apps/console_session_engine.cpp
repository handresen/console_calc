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
#include "console_calc/expression_parser.h"
#include "console_calc/scalar_value.h"
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
        ConsoleEngineCommandResult ignored;
        refresh_currency_rates(false, ignored);
    }
}

ConsoleEngineCommandResult ConsoleSessionEngine::submit(std::string_view line) {
    initialize();

    ConsoleEngineCommandResult result;
    const std::string trimmed = trim(line);
    if (trimmed.empty()) {
        return result;
    }

    if (try_handle_hidden_command(trimmed, result)) {
        return result;
    }

    const ConsoleCommand command = classify_console_command(trimmed);
    if (command.kind == ConsoleCommandKind::quit) {
        result.should_exit = true;
        return result;
    }

    try {
        if (command.kind == ConsoleCommandKind::refresh_currency_rates) {
            refresh_currency_rates(true, result);
            return result;
        }

        if (is_non_evaluating_console_command(command.kind)) {
            StringConsoleCommandExecutionContext context{
                .result_stack = result_stack_,
                .max_stack_depth = max_stack_depth_,
                .definitions = definitions_,
                .constants = constants_,
                .display_mode = display_mode_,
                .output_lines = result.output_lines,
                .mutable_stack = result_stack_,
            };
            execute_non_evaluating_console_command(command.kind, context);
            return result;
        }

        if (command.kind == ConsoleCommandKind::stack_operator) {
            const Value stack_result = apply_stack_operator(command.stack_operator);
            result.emitted_values.push_back(stack_result);
            return result;
        }

        const std::optional<Value> result_reference = top_result();
        if (command.kind == ConsoleCommandKind::assignment) {
            const auto assignment = parse_variable_assignment(trimmed);
            if (assignment.has_value()) {
                if (constants_.contains(assignment->name)) {
                    throw std::invalid_argument("cannot redefine constant: " + assignment->name);
                }
                assign_definition(assignment->name, assignment->expression, result_reference);
                return result;
            }
        }

        const Value evaluation_result = evaluate_expanded_expression(
            parser_, trimmed, constants_, definitions_, result_reference);
        push_result(evaluation_result);
        result.emitted_values.push_back(evaluation_result);
    } catch (const std::exception& ex) {
        result.error_lines.emplace_back(ex.what());
    }

    return result;
}

std::size_t ConsoleSessionEngine::stack_depth() const { return result_stack_.size(); }

IntegerDisplayMode ConsoleSessionEngine::display_mode() const { return display_mode_; }

std::span<const Value> ConsoleSessionEngine::stack() const { return result_stack_; }

const DefinitionTable& ConsoleSessionEngine::definitions() const { return definitions_; }

const ConstantTable& ConsoleSessionEngine::constants() const { return constants_; }

bool ConsoleSessionEngine::try_handle_hidden_command(std::string_view line,
                                                     ConsoleEngineCommandResult&) {
    constexpr std::string_view k_stack_depth_prefix = "stack_depth(";
    if (!line.starts_with(k_stack_depth_prefix) || !line.ends_with(')')) {
        return false;
    }

    const std::string depth_text =
        trim(line.substr(k_stack_depth_prefix.size(),
                         line.size() - k_stack_depth_prefix.size() - 1));
    if (depth_text.empty()) {
        throw std::invalid_argument("stack_depth() requires a positive integer");
    }

    std::size_t parsed_depth = 0;
    try {
        std::size_t consumed = 0;
        parsed_depth = std::stoull(depth_text, &consumed);
        if (consumed != depth_text.size()) {
            throw std::invalid_argument("invalid");
        }
    } catch (const std::exception&) {
        throw std::invalid_argument("stack_depth() requires a positive integer");
    }

    if (parsed_depth == 0) {
        throw std::invalid_argument("stack_depth() requires a positive integer");
    }

    set_stack_depth(parsed_depth);
    return true;
}

void ConsoleSessionEngine::assign_definition(std::string_view name, std::string_view expression,
                                             const std::optional<Value>& result_reference) {
    const std::string normalized_expression = normalize_assignment_expression(expression);
    DefinitionTable validation_definitions = definitions_;
    validation_definitions[std::string(name)] = UserDefinition{normalized_expression};
    const std::string expanded_expression = expand_expression_identifiers(
        normalized_expression, constants_, validation_definitions, result_reference);
    (void)parser_.evaluate_value(expanded_expression);
    definitions_[std::string(name)] = UserDefinition{normalized_expression};
}

void ConsoleSessionEngine::refresh_currency_rates(bool report_errors,
                                                  ConsoleEngineCommandResult&) {
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

void ConsoleSessionEngine::set_stack_depth(std::size_t depth) {
    max_stack_depth_ = depth;
    while (result_stack_.size() > max_stack_depth_) {
        result_stack_.erase(result_stack_.begin());
    }
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
