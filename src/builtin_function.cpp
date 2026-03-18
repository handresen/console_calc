#include "console_calc/builtin_function.h"

#include <array>
#include <stdexcept>

namespace console_calc {

namespace {

constexpr std::array<BuiltinFunctionInfo, 16> k_builtin_functions = {{
    {Function::sin, "sin", 1, BuiltinFunctionCategory::scalar, true, "sine in radians"},
    {Function::cos, "cos", 1, BuiltinFunctionCategory::scalar, true, "cosine in radians"},
    {Function::tan, "tan", 1, BuiltinFunctionCategory::scalar, true, "tangent in radians"},
    {Function::sind, "sind", 1, BuiltinFunctionCategory::scalar, true, "sine in degrees"},
    {Function::cosd, "cosd", 1, BuiltinFunctionCategory::scalar, true, "cosine in degrees"},
    {Function::tand, "tand", 1, BuiltinFunctionCategory::scalar, true, "tangent in degrees"},
    {Function::pow, "pow", 2, BuiltinFunctionCategory::scalar, false, "power"},
    {Function::sum, "sum", 1, BuiltinFunctionCategory::list, false, "sum list elements"},
    {Function::len, "len", 1, BuiltinFunctionCategory::list, false, "list length"},
    {Function::product, "product", 1, BuiltinFunctionCategory::list, false, "product of list elements"},
    {Function::avg, "avg", 1, BuiltinFunctionCategory::list, false, "average of list elements"},
    {Function::min, "min", 1, BuiltinFunctionCategory::list, false, "minimum list element"},
    {Function::max, "max", 1, BuiltinFunctionCategory::list, false, "maximum list element"},
    {Function::first, "first", 2, BuiltinFunctionCategory::list, false, "first n list elements"},
    {Function::drop, "drop", 2, BuiltinFunctionCategory::list, false, "drop first n list elements"},
    {Function::map, "map", 2, BuiltinFunctionCategory::list, false, "map unary scalar builtin over list"},
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

bool is_scalar_function(Function function) {
    return builtin_function_info(function).category == BuiltinFunctionCategory::scalar;
}

bool is_list_function(Function function) {
    return builtin_function_info(function).category == BuiltinFunctionCategory::list;
}

bool is_unary_scalar_function(Function function) {
    const auto& info = builtin_function_info(function);
    return info.category == BuiltinFunctionCategory::scalar && info.arity == 1;
}

bool is_mappable_unary_scalar_function(Function function) {
    const auto& info = builtin_function_info(function);
    return info.category == BuiltinFunctionCategory::scalar && info.arity == 1 && info.mappable;
}

}  // namespace console_calc
