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

[[nodiscard]] bool is_stack_operator(std::string_view text) {
    return text.size() == 1 &&
           (text[0] == '+' || text[0] == '-' || text[0] == '*' || text[0] == '/' ||
            text[0] == '%' || text[0] == '^' || text[0] == '&' || text[0] == '|');
}

}  // namespace

ConsoleSession::ConsoleSession(const ExpressionParser& parser, const ConstantTable& constants,
                               std::istream& input, std::ostream& output,
                               std::ostream& error)
    : parser_(parser), constants_(constants), input_(input), output_(output), error_(error) {}

int ConsoleSession::run() {
    std::string line;
    while (true) {
        print_prompt();
        output_.flush();

        if (!std::getline(input_, line)) {
            return 0;
        }

        const int result = handle_line(line);
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

    if (trimmed == "q" || trimmed == "Q") {
        return 0;
    }

    try {
        if (trimmed == "s" || trimmed == "consts" || trimmed == "dup" || trimmed == "drop" ||
            trimmed == "swap" || trimmed == "clear") {
            execute_stack_command(trimmed);
            return -1;
        }

        if (is_stack_operator(trimmed)) {
            output_ << apply_stack_operator(trimmed[0]) << '\n';
            return -1;
        }

        const std::optional<double> result_reference = result_stack_.empty()
                                                           ? std::nullopt
                                                           : std::optional<double>{result_stack_.back()};
        if (const auto assignment = parse_variable_assignment(trimmed)) {
            if (constants_.contains(assignment->name)) {
                throw std::invalid_argument("cannot redefine constant: " + assignment->name);
            }
            const std::string expanded_expression = expand_expression_identifiers(
                assignment->expression, constants_, variables_, result_reference);
            variables_[assignment->name] = evaluate_assignment_value(parser_, expanded_expression);
            return -1;
        }

        const double result = parser_.evaluate(
            expand_expression_identifiers(trimmed, constants_, variables_, result_reference));
        if (result_stack_.size() >= k_max_stack_depth) {
            throw std::invalid_argument("stack is full");
        }
        result_stack_.push_back(result);
        output_ << result << '\n';
    } catch (const std::exception& ex) {
        error_ << "error: " << ex.what() << '\n';
    }

    return -1;
}

void ConsoleSession::print_prompt() const {
    output_ << k_prompt_color << result_stack_.size() << '>' << k_color_reset;
}

void ConsoleSession::print_stack() const {
    for (std::size_t index = 0; index < result_stack_.size(); ++index) {
        output_ << index << ':' << format_scalar(result_stack_[index]) << '\n';
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

void ConsoleSession::execute_stack_command(std::string_view command) {
    if (command == "s") {
        print_stack();
        return;
    }
    if (command == "consts") {
        print_constants();
        return;
    }
    if (command == "dup") {
        if (result_stack_.empty()) {
            throw std::invalid_argument("stack requires at least one value");
        }
        if (result_stack_.size() >= k_max_stack_depth) {
            throw std::invalid_argument("stack is full");
        }
        result_stack_.push_back(result_stack_.back());
        return;
    }
    if (command == "drop") {
        if (result_stack_.empty()) {
            throw std::invalid_argument("stack requires at least one value");
        }
        result_stack_.pop_back();
        return;
    }
    if (command == "swap") {
        if (result_stack_.size() < 2) {
            throw std::invalid_argument("stack requires at least two values");
        }
        std::swap(result_stack_[result_stack_.size() - 1], result_stack_[result_stack_.size() - 2]);
        return;
    }
    if (command == "clear") {
        result_stack_.clear();
    }
}

double ConsoleSession::apply_stack_operator(char op) {
    if (result_stack_.size() < 2) {
        throw std::invalid_argument("stack requires at least two values");
    }

    const double rhs = result_stack_.back();
    result_stack_.pop_back();
    const double lhs = result_stack_.back();
    result_stack_.pop_back();

    const std::string expression =
        format_scalar(lhs) + ' ' + op + ' ' + format_scalar(rhs);
    try {
        const double result = parser_.evaluate(expression);
        result_stack_.push_back(result);
        return result;
    } catch (...) {
        result_stack_.push_back(lhs);
        result_stack_.push_back(rhs);
        throw;
    }
}

}  // namespace console_calc
