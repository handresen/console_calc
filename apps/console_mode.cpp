#include "console_mode.h"

#include <cctype>
#include <exception>
#include <iomanip>
#include <istream>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "console_calc/expression_parser.h"

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

[[nodiscard]] bool is_stack_operator(std::string_view text) {
    return text.size() == 1 &&
           (text[0] == '+' || text[0] == '-' || text[0] == '*' || text[0] == '/' ||
            text[0] == '%' || text[0] == '^' || text[0] == '&' || text[0] == '|');
}

[[nodiscard]] std::string format_number(double value) {
    std::ostringstream stream;
    stream << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
    return stream.str();
}

void print_stack(const std::vector<double>& result_stack, std::ostream& output) {
    for (std::size_t index = 0; index < result_stack.size(); ++index) {
        output << index << ':' << format_number(result_stack[index]) << '\n';
    }
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

}  // namespace

int run_console_mode(const ExpressionParser& parser, std::istream& input, std::ostream& output,
                     std::ostream& error) {
    std::vector<double> result_stack;
    std::string line;
    while (true) {
        output << result_stack.size() << ':';
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
            if (trimmed == "s") {
                print_stack(result_stack, output);
                continue;
            }

            if (is_stack_operator(trimmed)) {
                output << apply_stack_operator(parser, result_stack, trimmed[0]) << '\n';
                continue;
            }

            const double result = parser.evaluate(trimmed);
            result_stack.push_back(result);
            output << result << '\n';
        } catch (const std::exception& ex) {
            error << "error: " << ex.what() << '\n';
        }
    }
}

}  // namespace console_calc
