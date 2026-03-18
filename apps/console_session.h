#pragma once

#include <iosfwd>
#include <optional>
#include <string_view>
#include <vector>

#include "expression_environment.h"

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
    void print_stack() const;
    void print_constants() const;
    void execute_stack_command(std::string_view command);
    double apply_stack_operator(char op);

    const ExpressionParser& parser_;
    const ConstantTable& constants_;
    std::istream& input_;
    std::ostream& output_;
    std::ostream& error_;
    std::vector<double> result_stack_;
    VariableTable variables_;
};

}  // namespace console_calc
