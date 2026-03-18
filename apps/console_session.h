#pragma once

#include <iosfwd>
#include <optional>
#include <string_view>
#include <vector>

#include "console_command.h"
#include "console_history.h"
#include "console_line_editor.h"
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
    int handle_line(std::string_view line);
    void print_prompt() const;
    [[nodiscard]] std::string prompt_text() const;
    void print_stack() const;
    void print_variables() const;
    void print_constants() const;
    void print_functions() const;
    void print_result(const Value& value);
    void execute_stack_command(ConsoleCommandKind command);
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
    ConsoleLineEditor line_editor_;
};

}  // namespace console_calc
