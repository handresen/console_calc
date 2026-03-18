#include "console_session.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <iostream>
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <termios.h>
#include <unistd.h>
#endif

#include "console_assignment.h"
#include "console_calc/expression_parser.h"
#include "console_calc/value_format.h"

namespace console_calc {

namespace {

constexpr std::size_t k_max_stack_depth = 9;
constexpr std::string_view k_prompt_color = "\x1b[32m";
constexpr std::string_view k_color_reset = "\x1b[0m";
constexpr std::string_view k_clear_line = "\r\x1b[2K";

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

[[nodiscard]] bool use_interactive_input(const std::istream& input) {
#if defined(__unix__) || defined(__APPLE__)
    return &input == &std::cin && ::isatty(STDIN_FILENO) != 0;
#else
    (void)input;
    return false;
#endif
}

#if defined(__unix__) || defined(__APPLE__)
class TerminalRawModeGuard {
public:
    explicit TerminalRawModeGuard(bool enabled) : enabled_(enabled) {
        if (!enabled_) {
            return;
        }

        if (::tcgetattr(STDIN_FILENO, &original_) != 0) {
            enabled_ = false;
            return;
        }

        termios raw = original_;
        raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        if (::tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) {
            enabled_ = false;
        }
    }

    ~TerminalRawModeGuard() {
        if (enabled_) {
            (void)::tcsetattr(STDIN_FILENO, TCSANOW, &original_);
        }
    }

    TerminalRawModeGuard(const TerminalRawModeGuard&) = delete;
    TerminalRawModeGuard& operator=(const TerminalRawModeGuard&) = delete;

private:
    bool enabled_;
    termios original_{};
};
#endif

}  // namespace

ConsoleSession::ConsoleSession(const ExpressionParser& parser, const ConstantTable& constants,
                               std::istream& input, std::ostream& output,
                               std::ostream& error)
    : parser_(parser), constants_(constants), input_(input), output_(output), error_(error) {}

int ConsoleSession::run() {
#if defined(__unix__) || defined(__APPLE__)
    TerminalRawModeGuard raw_mode(use_interactive_input(input_));
#endif
    while (true) {
        const auto line = read_command_line();
        if (!line.has_value()) {
            return 0;
        }

        const int result = handle_line(*line);
        if (result >= 0) {
            return result;
        }
    }
}

std::optional<std::string> ConsoleSession::read_command_line() {
    print_prompt();
    output_.flush();

    if (!use_interactive_input(input_)) {
        std::string line;
        if (!std::getline(input_, line)) {
            return std::nullopt;
        }
        return line;
    }

    history_.reset_navigation();
    std::string buffer;
    std::size_t cursor = 0;
    while (true) {
        const int next = input_.get();
        if (next == EOF) {
            return std::nullopt;
        }

        const char ch = static_cast<char>(next);
        if (ch == '\n' || ch == '\r') {
            output_ << '\n';
            history_.record(buffer);
            return buffer;
        }

        if (ch == '\x7f' || ch == '\b') {
            if (cursor > 0) {
                buffer.erase(cursor - 1, 1);
                --cursor;
                redraw_input_line(buffer, cursor);
            }
            continue;
        }

        if (ch == '\x1b') {
            if (input_.peek() == '[') {
                (void)input_.get();
                const int code = input_.get();
                if (code == 'A') {
                    if (const auto previous = history_.previous()) {
                        buffer = *previous;
                        cursor = buffer.size();
                        redraw_input_line(buffer, cursor);
                    }
                } else if (code == 'C') {
                    if (cursor < buffer.size()) {
                        ++cursor;
                        redraw_input_line(buffer, cursor);
                    }
                } else if (code == 'D') {
                    if (cursor > 0) {
                        --cursor;
                        redraw_input_line(buffer, cursor);
                    }
                }
            }
            continue;
        }

        if (!std::isprint(static_cast<unsigned char>(ch))) {
            continue;
        }

        buffer.insert(buffer.begin() + static_cast<std::ptrdiff_t>(cursor), ch);
        ++cursor;
        redraw_input_line(buffer, cursor);
    }
}

void ConsoleSession::redraw_input_line(std::string_view buffer, std::size_t cursor) const {
    output_ << k_clear_line;
    print_prompt();
    output_ << buffer;
    if (cursor < buffer.size()) {
        output_ << '\r';
        print_prompt();
        output_ << buffer.substr(0, cursor);
    }
    output_.flush();
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

        const std::optional<Value> result_reference = top_result();
        if (const auto assignment = parse_variable_assignment(trimmed)) {
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

        const Value result = parser_.evaluate_value(
            expand_expression_identifiers(trimmed, constants_, variables_, result_reference));
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

void ConsoleSession::print_prompt() const {
    output_ << k_prompt_color << result_stack_.size() << '>' << k_color_reset;
}

void ConsoleSession::print_stack() const {
    for (std::size_t index = 0; index < result_stack_.size(); ++index) {
        output_ << index << ':' << format_value(result_stack_[index]) << '\n';
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

void ConsoleSession::print_result(const Value& value) {
    if (const auto* scalar = std::get_if<double>(&value)) {
        output_ << *scalar << '\n';
        return;
    }

    output_ << format_value(value) << '\n';
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
