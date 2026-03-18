#include "console_mode.h"

#include <cctype>
#include <algorithm>
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

#include "expression_environment.h"
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

struct VariableAssignment {
    std::string name;
    std::string expression;
};

[[nodiscard]] std::vector<std::string> split_top_level_items(std::string_view text) {
    std::vector<std::string> items;
    std::size_t item_begin = 0;
    int paren_depth = 0;
    int brace_depth = 0;

    for (std::size_t index = 0; index < text.size(); ++index) {
        switch (text[index]) {
        case '(':
            ++paren_depth;
            break;
        case ')':
            --paren_depth;
            break;
        case '{':
            ++brace_depth;
            break;
        case '}':
            --brace_depth;
            break;
        case ',':
            if (paren_depth == 0 && brace_depth == 0) {
                items.push_back(trim(text.substr(item_begin, index - item_begin)));
                item_begin = index + 1;
            }
            break;
        default:
            break;
        }
    }

    items.push_back(trim(text.substr(item_begin)));
    return items;
}

[[nodiscard]] bool is_braced_list_literal(std::string_view text) {
    if (text.size() < 2 || text.front() != '{' || text.back() != '}') {
        return false;
    }

    int depth = 0;
    for (std::size_t index = 0; index < text.size(); ++index) {
        if (text[index] == '{') {
            ++depth;
        } else if (text[index] == '}') {
            --depth;
            if (depth == 0 && index + 1 != text.size()) {
                return false;
            }
        }

        if (depth < 0) {
            return false;
        }
    }

    return depth == 0;
}

[[nodiscard]] bool is_stack_operator(std::string_view text) {
    return text.size() == 1 &&
           (text[0] == '+' || text[0] == '-' || text[0] == '*' || text[0] == '/' ||
            text[0] == '%' || text[0] == '^' || text[0] == '&' || text[0] == '|');
}

enum class StackCommand {
    list,
    list_constants,
    duplicate,
    drop,
    swap,
    clear,
};

[[nodiscard]] std::optional<StackCommand> parse_stack_command(std::string_view text) {
    if (text == "s") {
        return StackCommand::list;
    }
    if (text == "consts") {
        return StackCommand::list_constants;
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

[[nodiscard]] std::optional<VariableAssignment> parse_variable_assignment(
    std::string_view text) {
    const std::size_t separator = text.find(':');
    if (separator == std::string_view::npos) {
        return std::nullopt;
    }

    const std::string name = trim(text.substr(0, separator));
    const std::string expression = trim(text.substr(separator + 1));
    if (!is_identifier(name)) {
        return std::nullopt;
    }
    if (expression.empty()) {
        throw std::invalid_argument("expected expression after ':'");
    }

    return VariableAssignment{name, expression};
}

[[nodiscard]] VariableValue evaluate_assignment_value(const ExpressionParser& parser,
                                                      std::string_view expression) {
    const std::string trimmed = trim(expression);
    if (is_braced_list_literal(trimmed)) {
        return parser.evaluate_value(trimmed);
    }

    const std::vector<std::string> items = split_top_level_items(trimmed);
    if (items.size() == 1) {
        return parser.evaluate_value(trimmed);
    }

    std::string list_expression = "{";
    for (std::size_t index = 0; index < items.size(); ++index) {
        if (items[index].empty()) {
            throw std::invalid_argument("expected expression after ','");
        }
        if (index != 0) {
            list_expression += ", ";
        }
        list_expression += items[index];
    }
    list_expression += '}';
    return parser.evaluate_value(list_expression);
}

void print_stack(const std::vector<double>& result_stack, std::ostream& output) {
    for (std::size_t index = 0; index < result_stack.size(); ++index) {
        output << index << ':' << format_number(result_stack[index]) << '\n';
    }
}

void print_constants(const ConstantTable& constants, std::ostream& output) {
    std::vector<std::string> names;
    names.reserve(constants.size());
    for (const auto& [name, _] : constants) {
        names.push_back(name);
    }

    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        output << name << ':' << format_number(constants.at(name)) << '\n';
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

void execute_stack_command(StackCommand command, const ConstantTable& constants,
                           std::vector<double>& result_stack, std::ostream& output) {
    switch (command) {
    case StackCommand::list:
        print_stack(result_stack, output);
        return;
    case StackCommand::list_constants:
        print_constants(constants, output);
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

int run_console_mode(const ExpressionParser& parser, const ConstantTable& constants,
                     std::istream& input, std::ostream& output, std::ostream& error) {
    std::vector<double> result_stack;
    VariableTable variables;
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
                execute_stack_command(*command, constants, result_stack, output);
                continue;
            }

            if (is_stack_operator(trimmed)) {
                output << apply_stack_operator(parser, result_stack, trimmed[0]) << '\n';
                continue;
            }

            const std::optional<double> result_reference =
                result_stack.empty() ? std::nullopt : std::optional<double>{result_stack.back()};
            if (const auto assignment = parse_variable_assignment(trimmed)) {
                if (constants.contains(assignment->name)) {
                    throw std::invalid_argument("cannot redefine constant: " + assignment->name);
                }
                const std::string expanded_expression = expand_expression_identifiers(
                    assignment->expression, constants, variables, result_reference);
                variables[assignment->name] =
                    evaluate_assignment_value(parser, expanded_expression);
                continue;
            }

            const double result = parser.evaluate(
                expand_expression_identifiers(trimmed, constants, variables, result_reference));
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
