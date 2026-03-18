#include "console_calc/builtin_function.h"

#include <array>
#include <stdexcept>

namespace console_calc {

namespace {

constexpr std::array<BuiltinFunctionInfo, 15> k_builtin_functions = {{
    {Function::sin, "sin", 1, BuiltinFunctionCategory::scalar, "sine in radians"},
    {Function::cos, "cos", 1, BuiltinFunctionCategory::scalar, "cosine in radians"},
    {Function::tan, "tan", 1, BuiltinFunctionCategory::scalar, "tangent in radians"},
    {Function::sind, "sind", 1, BuiltinFunctionCategory::scalar, "sine in degrees"},
    {Function::cosd, "cosd", 1, BuiltinFunctionCategory::scalar, "cosine in degrees"},
    {Function::tand, "tand", 1, BuiltinFunctionCategory::scalar, "tangent in degrees"},
    {Function::pow, "pow", 2, BuiltinFunctionCategory::scalar, "power"},
    {Function::sum, "sum", 1, BuiltinFunctionCategory::list, "sum list elements"},
    {Function::len, "len", 1, BuiltinFunctionCategory::list, "list length"},
    {Function::product, "product", 1, BuiltinFunctionCategory::list, "product of list elements"},
    {Function::avg, "avg", 1, BuiltinFunctionCategory::list, "average of list elements"},
    {Function::min, "min", 1, BuiltinFunctionCategory::list, "minimum list element"},
    {Function::max, "max", 1, BuiltinFunctionCategory::list, "maximum list element"},
    {Function::first, "first", 2, BuiltinFunctionCategory::list, "first n list elements"},
    {Function::drop, "drop", 2, BuiltinFunctionCategory::list, "drop first n list elements"},
}};

}  // namespace

std::optional<Function> parse_builtin_function(std::string_view name) {
    for (const auto& function : k_builtin_functions) {
        if (function.name == name) {
            return function.function;
        }
    }

    return std::nullopt;
}

const BuiltinFunctionInfo& builtin_function_info(Function function) {
    for (const auto& info : k_builtin_functions) {
        if (info.function == function) {
            return info;
        }
    }

    throw std::invalid_argument("unknown builtin function");
}

std::string_view builtin_function_name(Function function) {
    return builtin_function_info(function).name;
}

std::size_t builtin_function_arity(Function function) {
    return builtin_function_info(function).arity;
}

bool is_builtin_function_name(std::string_view name) {
    return parse_builtin_function(name).has_value();
}

std::span<const BuiltinFunctionInfo> builtin_functions() {
    return k_builtin_functions;
}

bool is_list_function(Function function) {
    return builtin_function_info(function).category == BuiltinFunctionCategory::list;
}

}  // namespace console_calc
