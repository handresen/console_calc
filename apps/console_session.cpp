#include "console_session.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "console_assignment.h"
#include "console_listing.h"
#include "console_calc/builtin_function.h"
#include "console_command_executor.h"
#include "console_calc/expression_parser.h"
#include "console_calc/scalar_value.h"
#include "console_calc/value_format.h"
#include "scalar_math.h"

namespace console_calc {

namespace {

constexpr std::string_view k_prompt_color = "\x1b[32m";
constexpr std::string_view k_color_reset = "\x1b[0m";
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

ConsoleSession::ConsoleSession(const ExpressionParser& parser, const ConstantTable& constants,
                               std::istream& input, std::ostream& output,
                               std::ostream& error)
    : parser_(parser),
      constants_(constants),
      input_(input),
      output_(output),
      error_(error),
      line_editor_(input_, output_, history_) {}

int ConsoleSession::run() {
    while (true) {
        const auto line = line_editor_.read_line(prompt_text());
        if (!line.has_value()) {
            return 0;
        }

        const int result = handle_line(*line);
        if (result >= 0) {
            return result;
        }
    }
}

int ConsoleSession::handle_line(std::string_view line) {
    const std::string trimmed = trim(line);
    if (trimmed.empty()) {
        return -1;
    }

    if (try_handle_hidden_command(trimmed)) {
        return -1;
    }

    const ConsoleCommand command = classify_console_command(trimmed);
    if (command.kind == ConsoleCommandKind::quit) {
        return 0;
    }

    try {
        if (is_non_evaluating_console_command(command.kind)) {
            execute_non_evaluating_console_command(
                command.kind,
                ConsoleCommandExecutionContext{
                    .result_stack = result_stack_,
                    .max_stack_depth = max_stack_depth_,
                    .definitions = definitions_,
                    .constants = constants_,
                    .display_mode = display_mode_,
                    .output = output_,
                    .mutable_stack = result_stack_,
                });
            return -1;
        }

        if (command.kind == ConsoleCommandKind::stack_operator) {
            print_result(apply_stack_operator(command.stack_operator));
            return -1;
        }

        const std::optional<Value> result_reference = top_result();
        if (command.kind == ConsoleCommandKind::assignment) {
            const auto assignment = parse_variable_assignment(trimmed);
            if (assignment.has_value()) {
                if (constants_.contains(assignment->name)) {
                    throw std::invalid_argument("cannot redefine constant: " + assignment->name);
                }
                assign_definition(assignment->name, assignment->expression, result_reference);
                return -1;
            }
        }

        const Value result = evaluate_expanded_expression(
            parser_, trimmed, constants_, definitions_, result_reference);
        push_result(result);
        print_result(result);
    } catch (const std::exception& ex) {
        error_ << "error: " << ex.what() << '\n';
    }

    return -1;
}

std::string ConsoleSession::prompt_text() const {
    return std::string(k_prompt_color) + std::to_string(result_stack_.size()) + '>' +
           std::string(k_color_reset);
}

void ConsoleSession::print_result(const Value& value) {
    if (const auto* integer = std::get_if<std::int64_t>(&value)) {
        output_ << format_scalar(ScalarValue{*integer}, display_mode_) << '\n';
        return;
    }

    if (const auto* scalar = std::get_if<double>(&value)) {
        output_ << *scalar << '\n';
        return;
    }

    output_ << format_console_value(value, display_mode_) << '\n';
}

bool ConsoleSession::try_handle_hidden_command(std::string_view line) {
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

void ConsoleSession::assign_definition(std::string_view name, std::string_view expression,
                                       const std::optional<Value>& result_reference) {
    const std::string normalized_expression = normalize_assignment_expression(expression);
    DefinitionTable validation_definitions = definitions_;
    validation_definitions[std::string(name)] = UserDefinition{normalized_expression};
    const std::string expanded_expression = expand_expression_identifiers(
        normalized_expression, constants_, validation_definitions, result_reference);
    (void)parser_.evaluate_value(expanded_expression);
    definitions_[std::string(name)] = UserDefinition{normalized_expression};
}

void ConsoleSession::push_result(Value result) {
    if (result_stack_.size() >= max_stack_depth_) {
        result_stack_.erase(result_stack_.begin());
    }

    result_stack_.push_back(std::move(result));
}

void ConsoleSession::set_stack_depth(std::size_t depth) {
    max_stack_depth_ = depth;
    while (result_stack_.size() > max_stack_depth_) {
        result_stack_.erase(result_stack_.begin());
    }
}

Value ConsoleSession::apply_stack_operator(char op) {
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

std::optional<Value> ConsoleSession::top_result() const {
    if (result_stack_.empty()) {
        return std::nullopt;
    }

    return result_stack_.back();
}

}  // namespace console_calc
