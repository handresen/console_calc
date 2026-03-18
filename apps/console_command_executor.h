#pragma once

#include <cstddef>
#include <iosfwd>
#include <span>
#include <vector>

#include "console_command.h"
#include "expression_environment.h"
#include "console_calc/value.h"
#include "console_calc/value_format.h"

namespace console_calc {

struct ConsoleCommandExecutionContext {
    std::span<const Value> result_stack;
    std::size_t& max_stack_depth;
    const DefinitionTable& definitions;
    const ConstantTable& constants;
    IntegerDisplayMode& display_mode;
    std::ostream& output;
    std::vector<Value>& mutable_stack;
};

[[nodiscard]] bool is_non_evaluating_console_command(ConsoleCommandKind command);
void execute_non_evaluating_console_command(
    ConsoleCommandKind command, const ConsoleCommandExecutionContext& context);

}  // namespace console_calc
