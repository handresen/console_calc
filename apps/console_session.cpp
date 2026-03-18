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
#include "console_calc/expression_parser.h"
#include "console_calc/value_format.h"

namespace console_calc {

namespace {

constexpr std::size_t k_max_stack_depth = 9;
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

    const ConsoleCommand command = classify_console_command(trimmed);
    if (command.kind == ConsoleCommandKind::quit) {
        return 0;
    }

    try {
        if (command.kind == ConsoleCommandKind::list_stack ||
            command.kind == ConsoleCommandKind::list_variables ||
            command.kind == ConsoleCommandKind::list_constants ||
            command.kind == ConsoleCommandKind::list_functions ||
            command.kind == ConsoleCommandKind::display_decimal ||
            command.kind == ConsoleCommandKind::display_hexadecimal ||
            command.kind == ConsoleCommandKind::display_binary ||
            command.kind == ConsoleCommandKind::duplicate ||
            command.kind == ConsoleCommandKind::drop ||
            command.kind == ConsoleCommandKind::swap ||
            command.kind == ConsoleCommandKind::clear) {
            execute_stack_command(command.kind);
            return -1;
        }

        if (command.kind == ConsoleCommandKind::stack_operator) {
            output_ << apply_stack_operator(command.stack_operator) << '\n';
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

void ConsoleSession::print_prompt() const {
    output_ << prompt_text();
}

void ConsoleSession::print_stack() const {
    output_ << format_stack_listing(result_stack_, display_mode_);
}

void ConsoleSession::print_variables() const {
    output_ << format_definition_listing(definitions_);
}

void ConsoleSession::print_constants() const {
    output_ << format_constant_listing(constants_);
}

void ConsoleSession::print_functions() const {
    output_ << format_builtin_function_listing(builtin_functions());
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

    output_ << format_value(value, display_mode_) << '\n';
}

void ConsoleSession::execute_stack_command(ConsoleCommandKind command) {
    if (command == ConsoleCommandKind::list_stack) {
        print_stack();
        return;
    }
    if (command == ConsoleCommandKind::list_variables) {
        print_variables();
        return;
    }
    if (command == ConsoleCommandKind::list_constants) {
        print_constants();
        return;
    }
    if (command == ConsoleCommandKind::list_functions) {
        print_functions();
        return;
    }
    if (command == ConsoleCommandKind::display_decimal) {
        set_display_mode(IntegerDisplayMode::decimal);
        return;
    }
    if (command == ConsoleCommandKind::display_hexadecimal) {
        set_display_mode(IntegerDisplayMode::hexadecimal);
        return;
    }
    if (command == ConsoleCommandKind::display_binary) {
        set_display_mode(IntegerDisplayMode::binary);
        return;
    }
    if (command == ConsoleCommandKind::duplicate) {
        if (result_stack_.empty()) {
            throw std::invalid_argument("stack requires at least one value");
        }
        if (result_stack_.size() >= k_max_stack_depth) {
            throw std::invalid_argument("stack is full");
        }
        result_stack_.push_back(result_stack_.back());
        return;
    }
    if (command == ConsoleCommandKind::drop) {
        if (result_stack_.empty()) {
            throw std::invalid_argument("stack requires at least one value");
        }
        result_stack_.pop_back();
        return;
    }
    if (command == ConsoleCommandKind::swap) {
        if (result_stack_.size() < 2) {
            throw std::invalid_argument("stack requires at least two values");
        }
        std::swap(result_stack_[result_stack_.size() - 1], result_stack_[result_stack_.size() - 2]);
        return;
    }
    if (command == ConsoleCommandKind::clear) {
        result_stack_.clear();
    }
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
    if (result_stack_.size() >= k_max_stack_depth) {
        throw std::invalid_argument("stack is full");
    }

    result_stack_.push_back(std::move(result));
}

void ConsoleSession::set_display_mode(IntegerDisplayMode mode) {
    display_mode_ = mode;
}

double ConsoleSession::apply_stack_operator(char op) {
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

    const std::string expression = format_scalar(lhs) + ' ' + op + ' ' + format_scalar(rhs);
    try {
        const Value result = parser_.evaluate_value(expression);
        result_stack_.push_back(result);
        if (const auto* integer = std::get_if<std::int64_t>(&result)) {
            return static_cast<double>(*integer);
        }
        return std::get<double>(result);
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
