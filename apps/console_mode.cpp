#include "console_mode.h"

#include <cctype>
#include <exception>
#include <iomanip>
#include <istream>
#include <limits>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "console_calc/expression_parser.h"

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

enum class StackCommand {
    list,
    duplicate,
    drop,
    swap,
    clear,
};

[[nodiscard]] std::optional<StackCommand> parse_stack_command(std::string_view text) {
    if (text == "s") {
        return StackCommand::list;
    }
    if (text == "dup") {
        return StackCommand::duplicate;
    }
    if (text == "drop") {
        return StackCommand::drop;
    }
    if (text == "swap") {
        return StackCommand::swap;
    }
    if (text == "clear") {
        return StackCommand::clear;
    }

    return std::nullopt;
}

[[nodiscard]] std::string format_number(double value) {
    std::ostringstream stream;
    stream << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
    return stream.str();
}

[[nodiscard]] std::string expand_result_reference(std::string_view expression,
                                                  const std::vector<double>& result_stack) {
    if (expression.find('r') == std::string_view::npos) {
        return std::string(expression);
    }

    if (result_stack.empty()) {
        throw std::invalid_argument("result reference requires at least one value");
    }

    const std::string replacement = format_number(result_stack.back());
    std::string expanded;
    expanded.reserve(expression.size() + replacement.size());

    for (const char ch : expression) {
        if (ch == 'r') {
            expanded += replacement;
        } else {
            expanded += ch;
        }
    }

    return expanded;
}

void print_stack(const std::vector<double>& result_stack, std::ostream& output) {
    for (std::size_t index = 0; index < result_stack.size(); ++index) {
        output << index << ':' << format_number(result_stack[index]) << '\n';
    }
}

void print_prompt(std::size_t stack_depth, std::ostream& output) {
    output << k_prompt_color << stack_depth << '>' << k_color_reset;
}

double apply_stack_operator(const ExpressionParser& parser, std::vector<double>& result_stack,
                            char op) {
    if (result_stack.size() < 2) {
        throw std::invalid_argument("stack requires at least two values");
    }

    const double rhs = result_stack.back();
    result_stack.pop_back();
    const double lhs = result_stack.back();
    result_stack.pop_back();

    const std::string expression =
        format_number(lhs) + ' ' + op + ' ' + format_number(rhs);
    try {
        const double result = parser.evaluate(expression);
        result_stack.push_back(result);
        return result;
    } catch (...) {
        result_stack.push_back(lhs);
        result_stack.push_back(rhs);
        throw;
    }
}

void execute_stack_command(StackCommand command, std::vector<double>& result_stack,
                           std::ostream& output) {
    switch (command) {
    case StackCommand::list:
        print_stack(result_stack, output);
        return;
    case StackCommand::duplicate:
        if (result_stack.empty()) {
            throw std::invalid_argument("stack requires at least one value");
        }
        if (result_stack.size() >= k_max_stack_depth) {
            throw std::invalid_argument("stack is full");
        }
        result_stack.push_back(result_stack.back());
        return;
    case StackCommand::drop:
        if (result_stack.empty()) {
            throw std::invalid_argument("stack requires at least one value");
        }
        result_stack.pop_back();
        return;
    case StackCommand::swap:
        if (result_stack.size() < 2) {
            throw std::invalid_argument("stack requires at least two values");
        }
        std::swap(result_stack[result_stack.size() - 1], result_stack[result_stack.size() - 2]);
        return;
    case StackCommand::clear:
        result_stack.clear();
        return;
    }
}

}  // namespace

int run_console_mode(const ExpressionParser& parser, std::istream& input, std::ostream& output,
                     std::ostream& error) {
    std::vector<double> result_stack;
    std::string line;
    while (true) {
        print_prompt(result_stack.size(), output);
        output.flush();

        if (!std::getline(input, line)) {
            return 0;
        }

        const std::string trimmed = trim(line);
        if (trimmed.empty()) {
            continue;
        }

        if (trimmed == "q" || trimmed == "Q") {
            return 0;
        }

        try {
            if (const auto command = parse_stack_command(trimmed)) {
                execute_stack_command(*command, result_stack, output);
                continue;
            }

            if (is_stack_operator(trimmed)) {
                output << apply_stack_operator(parser, result_stack, trimmed[0]) << '\n';
                continue;
            }

            const double result =
                parser.evaluate(expand_result_reference(trimmed, result_stack));
            if (result_stack.size() >= k_max_stack_depth) {
                throw std::invalid_argument("stack is full");
            }
            result_stack.push_back(result);
            output << result << '\n';
        } catch (const std::exception& ex) {
            error << "error: " << ex.what() << '\n';
        }
    }
}

}  // namespace console_calc
