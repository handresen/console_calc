#pragma once

#include <iosfwd>
#include <optional>
#include <string_view>
#include <vector>

#include "console_history.h"
#include "expression_environment.h"
#include "console_calc/value.h"

namespace console_calc {

class ExpressionParser;

class ConsoleSession {
public:
    ConsoleSession(const ExpressionParser& parser, const ConstantTable& constants,
                   std::istream& input, std::ostream& output, std::ostream& error);

    int run();

private:
    std::optional<std::string> read_command_line();
    void redraw_input_line(std::string_view buffer, std::size_t cursor) const;
    int handle_line(std::string_view line);
    void print_prompt() const;
    void print_stack() const;
    void print_constants() const;
    void print_result(const Value& value);
    void execute_stack_command(std::string_view command);
    double apply_stack_operator(char op);
    std::optional<Value> top_result() const;

    const ExpressionParser& parser_;
    const ConstantTable& constants_;
    std::istream& input_;
    std::ostream& output_;
    std::ostream& error_;
    std::vector<Value> result_stack_;
    VariableTable variables_;
    ConsoleHistory history_;
};

}  // namespace console_calc
