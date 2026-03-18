#pragma once

#include <optional>
#include <string_view>

namespace console_calc {

enum class ConsoleCommandKind {
    quit,
    list_stack,
    list_variables,
    list_constants,
    list_functions,
    duplicate,
    drop,
    swap,
    clear,
    stack_operator,
    assignment,
    expression,
};

struct ConsoleCommand {
    ConsoleCommandKind kind;
    char stack_operator = '\0';
};

[[nodiscard]] ConsoleCommand classify_console_command(std::string_view text);

}  // namespace console_calc
