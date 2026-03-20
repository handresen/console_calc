#include "console_command_executor.h"

#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <utility>

#include "console_listing.h"
#include "console_calc/builtin_function.h"
#include "console_calc/special_form.h"

namespace console_calc {

bool is_non_evaluating_console_command(ConsoleCommandKind command) {
    switch (command) {
    case ConsoleCommandKind::list_stack:
    case ConsoleCommandKind::list_variables:
    case ConsoleCommandKind::list_constants:
    case ConsoleCommandKind::list_functions:
    case ConsoleCommandKind::display_decimal:
    case ConsoleCommandKind::display_hexadecimal:
    case ConsoleCommandKind::display_binary:
    case ConsoleCommandKind::duplicate:
    case ConsoleCommandKind::drop:
    case ConsoleCommandKind::swap:
    case ConsoleCommandKind::clear:
        return true;
    case ConsoleCommandKind::quit:
    case ConsoleCommandKind::refresh_currency_rates:
    case ConsoleCommandKind::stack_operator:
    case ConsoleCommandKind::assignment:
    case ConsoleCommandKind::expression:
        return false;
    }

    return false;
}

void execute_non_evaluating_console_command(
    ConsoleCommandKind command, const ConsoleCommandExecutionContext& context) {
    switch (command) {
    case ConsoleCommandKind::list_stack:
        context.output << format_stack_listing(context.result_stack, context.display_mode);
        return;
    case ConsoleCommandKind::list_variables:
        context.output << format_definition_listing(context.definitions);
        return;
    case ConsoleCommandKind::list_constants:
        context.output << format_constant_listing(context.constants);
        return;
    case ConsoleCommandKind::list_functions:
        context.output << format_function_listing(builtin_functions(), special_forms());
        return;
    case ConsoleCommandKind::display_decimal:
        context.display_mode = IntegerDisplayMode::decimal;
        return;
    case ConsoleCommandKind::display_hexadecimal:
        context.display_mode = IntegerDisplayMode::hexadecimal;
        return;
    case ConsoleCommandKind::display_binary:
        context.display_mode = IntegerDisplayMode::binary;
        return;
    case ConsoleCommandKind::duplicate:
        if (context.mutable_stack.empty()) {
            throw std::invalid_argument("stack requires at least one value");
        }
        if (context.mutable_stack.size() >= context.max_stack_depth) {
            context.mutable_stack.erase(context.mutable_stack.begin());
        }
        context.mutable_stack.push_back(context.mutable_stack.back());
        return;
    case ConsoleCommandKind::drop:
        if (context.mutable_stack.empty()) {
            throw std::invalid_argument("stack requires at least one value");
        }
        context.mutable_stack.pop_back();
        return;
    case ConsoleCommandKind::swap:
        if (context.mutable_stack.size() < 2) {
            throw std::invalid_argument("stack requires at least two values");
        }
        std::swap(context.mutable_stack[context.mutable_stack.size() - 1],
                  context.mutable_stack[context.mutable_stack.size() - 2]);
        return;
    case ConsoleCommandKind::clear:
        context.mutable_stack.clear();
        return;
    case ConsoleCommandKind::quit:
    case ConsoleCommandKind::refresh_currency_rates:
    case ConsoleCommandKind::stack_operator:
    case ConsoleCommandKind::assignment:
    case ConsoleCommandKind::expression:
        break;
    }

    throw std::invalid_argument("unsupported console command");
}

void execute_non_evaluating_console_command(
    ConsoleCommandKind command, const StringConsoleCommandExecutionContext& context) {
    switch (command) {
    case ConsoleCommandKind::list_stack:
        context.output_lines.push_back(
            format_stack_listing(context.result_stack, context.display_mode));
        return;
    case ConsoleCommandKind::list_variables:
        context.output_lines.push_back(format_definition_listing(context.definitions));
        return;
    case ConsoleCommandKind::list_constants:
        context.output_lines.push_back(format_constant_listing(context.constants));
        return;
    case ConsoleCommandKind::list_functions:
        context.output_lines.push_back(
            format_function_listing(builtin_functions(), special_forms()));
        return;
    case ConsoleCommandKind::display_decimal:
        context.display_mode = IntegerDisplayMode::decimal;
        return;
    case ConsoleCommandKind::display_hexadecimal:
        context.display_mode = IntegerDisplayMode::hexadecimal;
        return;
    case ConsoleCommandKind::display_binary:
        context.display_mode = IntegerDisplayMode::binary;
        return;
    case ConsoleCommandKind::duplicate:
        if (context.mutable_stack.empty()) {
            throw std::invalid_argument("stack requires at least one value");
        }
        if (context.mutable_stack.size() >= context.max_stack_depth) {
            context.mutable_stack.erase(context.mutable_stack.begin());
        }
        context.mutable_stack.push_back(context.mutable_stack.back());
        return;
    case ConsoleCommandKind::drop:
        if (context.mutable_stack.empty()) {
            throw std::invalid_argument("stack requires at least one value");
        }
        context.mutable_stack.pop_back();
        return;
    case ConsoleCommandKind::swap:
        if (context.mutable_stack.size() < 2) {
            throw std::invalid_argument("stack requires at least two values");
        }
        std::swap(context.mutable_stack[context.mutable_stack.size() - 1],
                  context.mutable_stack[context.mutable_stack.size() - 2]);
        return;
    case ConsoleCommandKind::clear:
        context.mutable_stack.clear();
        return;
    case ConsoleCommandKind::quit:
    case ConsoleCommandKind::refresh_currency_rates:
    case ConsoleCommandKind::stack_operator:
    case ConsoleCommandKind::assignment:
    case ConsoleCommandKind::expression:
        break;
    }

    throw std::invalid_argument("unsupported console command");
}

}  // namespace console_calc
