#include "console_session.h"

#include <cctype>
#include <algorithm>
#include <exception>
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "console_assignment.h"
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
                const std::string normalized_expression =
                    normalize_assignment_expression(assignment->expression);
                VariableTable validation_variables = variables_;
                validation_variables[assignment->name] = normalized_expression;
                const std::string expanded_expression = expand_expression_identifiers(
                    normalized_expression, constants_, validation_variables, result_reference);
                (void)parser_.evaluate_value(expanded_expression);
                variables_[assignment->name] = normalized_expression;
                return -1;
            }
        }

        const Value result = evaluate_expanded_expression(
            parser_, trimmed, constants_, variables_, result_reference);
        if (result_stack_.size() >= k_max_stack_depth) {
            throw std::invalid_argument("stack is full");
        }
        result_stack_.push_back(result);
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
    for (std::size_t index = 0; index < result_stack_.size(); ++index) {
        output_ << index << ':' << format_value(result_stack_[index]) << '\n';
    }
}

void ConsoleSession::print_variables() const {
    std::vector<std::string> names;
    names.reserve(variables_.size());
    for (const auto& [name, _] : variables_) {
        names.push_back(name);
    }

    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        output_ << name << ':' << variables_.at(name) << '\n';
    }
}

void ConsoleSession::print_constants() const {
    std::vector<std::string> names;
    names.reserve(constants_.size());
    for (const auto& [name, _] : constants_) {
        names.push_back(name);
    }

    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        output_ << name << ':' << format_scalar(constants_.at(name)) << '\n';
    }
}

void ConsoleSession::print_functions() const {
    std::vector<std::string> scalar_names;
    std::vector<std::string> list_names;
    scalar_names.reserve(builtin_functions().size());
    list_names.reserve(builtin_functions().size());
    for (const Function function : builtin_functions()) {
        std::string name(builtin_function_name(function));
        if (is_list_function(function)) {
            list_names.push_back(std::move(name));
        } else {
            scalar_names.push_back(std::move(name));
        }
    }

    std::sort(scalar_names.begin(), scalar_names.end());
    std::sort(list_names.begin(), list_names.end());

    output_ << "Scalar functions\n";
    for (const auto& name : scalar_names) {
        const auto function = parse_builtin_function(name);
        output_ << name << '/' << builtin_function_arity(*function) << '\n';
    }

    output_ << "List functions\n";
    for (const auto& name : list_names) {
        const auto function = parse_builtin_function(name);
        output_ << name << '/' << builtin_function_arity(*function) << '\n';
    }
}

void ConsoleSession::print_result(const Value& value) {
    if (const auto* scalar = std::get_if<double>(&value)) {
        output_ << *scalar << '\n';
        return;
    }

    output_ << format_value(value) << '\n';
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

double ConsoleSession::apply_stack_operator(char op) {
    if (result_stack_.size() < 2) {
        throw std::invalid_argument("stack requires at least two values");
    }

    const Value rhs_value = result_stack_.back();
    result_stack_.pop_back();
    const Value lhs_value = result_stack_.back();
    result_stack_.pop_back();

    const auto* rhs = std::get_if<double>(&rhs_value);
    const auto* lhs = std::get_if<double>(&lhs_value);
    if (lhs == nullptr || rhs == nullptr) {
        result_stack_.push_back(lhs_value);
        result_stack_.push_back(rhs_value);
        throw std::invalid_argument("stack operator requires scalar values");
    }

    const std::string expression =
        format_scalar(*lhs) + ' ' + op + ' ' + format_scalar(*rhs);
    try {
        const double result = parser_.evaluate(expression);
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
