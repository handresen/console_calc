#pragma once

#include <span>
#include <string>

#include "expression_environment.h"
#include "console_calc/builtin_function.h"
#include "console_calc/value.h"
#include "console_calc/value_format.h"

namespace console_calc {

[[nodiscard]] std::string format_stack_listing(std::span<const Value> values);
[[nodiscard]] std::string format_stack_listing(std::span<const Value> values,
                                               IntegerDisplayMode mode);
[[nodiscard]] std::string format_console_value(const Value& value, IntegerDisplayMode mode);
[[nodiscard]] std::string format_definition_listing(const DefinitionTable& definitions);
[[nodiscard]] std::string format_constant_listing(const ConstantTable& constants);
[[nodiscard]] std::string format_builtin_function_listing(
    std::span<const BuiltinFunctionInfo> functions);

}  // namespace console_calc
