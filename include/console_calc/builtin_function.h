#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

#include "console_calc/expression_ast.h"

namespace console_calc {

[[nodiscard]] std::optional<Function> parse_builtin_function(std::string_view name);
[[nodiscard]] std::string_view builtin_function_name(Function function);
[[nodiscard]] std::size_t builtin_function_arity(Function function);
[[nodiscard]] bool is_builtin_function_name(std::string_view name);
[[nodiscard]] std::span<const Function> builtin_functions();
[[nodiscard]] bool is_list_function(Function function);

}  // namespace console_calc
