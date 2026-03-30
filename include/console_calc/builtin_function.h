#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

#include "console_calc/expression_ast.h"

namespace console_calc {

enum class BuiltinFunctionCategory {
    scalar,
    position,
    list,
    statistics,
    list_generation,
};

struct BuiltinFunctionInfo {
    Function function;
    std::string_view name;
    std::size_t min_arity;
    std::size_t max_arity;
    BuiltinFunctionCategory category;
    bool scalar_arguments = true;
    bool mappable = false;
    std::string_view signature;
    std::string_view summary;
};

[[nodiscard]] std::optional<Function> parse_builtin_function(std::string_view name);
[[nodiscard]] std::string_view builtin_function_name(Function function);
[[nodiscard]] bool builtin_function_accepts_arity(Function function, std::size_t arity);
[[nodiscard]] std::string builtin_function_arity_label(Function function);
[[nodiscard]] std::string_view builtin_function_signature(Function function);
[[nodiscard]] bool is_builtin_function_name(std::string_view name);
[[nodiscard]] const BuiltinFunctionInfo& builtin_function_info(Function function);
[[nodiscard]] std::span<const BuiltinFunctionInfo> builtin_functions();
[[nodiscard]] bool is_scalar_function(Function function);
[[nodiscard]] bool is_list_function(Function function);
[[nodiscard]] bool is_unary_scalar_function(Function function);
[[nodiscard]] bool is_mappable_unary_scalar_function(Function function);

}  // namespace console_calc
