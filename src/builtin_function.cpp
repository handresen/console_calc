#include "console_calc/builtin_function.h"

#include <array>
#include <string>
#include <stdexcept>

namespace console_calc {

namespace {

constexpr std::array<BuiltinFunctionInfo, 29> k_builtin_functions = {{
    {Function::abs, "abs", 1, 1, BuiltinFunctionCategory::scalar, true, "abs(x)", "absolute value"},
    {Function::sin, "sin", 1, 1, BuiltinFunctionCategory::scalar, true, "sin(x)", "sine in radians"},
    {Function::cos, "cos", 1, 1, BuiltinFunctionCategory::scalar, true, "cos(x)", "cosine in radians"},
    {Function::tan, "tan", 1, 1, BuiltinFunctionCategory::scalar, true, "tan(x)", "tangent in radians"},
    {Function::sind, "sind", 1, 1, BuiltinFunctionCategory::scalar, true, "sind(x)", "sine in degrees"},
    {Function::cosd, "cosd", 1, 1, BuiltinFunctionCategory::scalar, true, "cosd(x)", "cosine in degrees"},
    {Function::tand, "tand", 1, 1, BuiltinFunctionCategory::scalar, true, "tand(x)", "tangent in degrees"},
    {Function::sqrt, "sqrt", 1, 1, BuiltinFunctionCategory::scalar, true, "sqrt(x)", "square root"},
    {Function::pow, "pow", 2, 2, BuiltinFunctionCategory::scalar, false, "pow(x, y)", "power"},
    {Function::sum, "sum", 1, 1, BuiltinFunctionCategory::list, false, "sum(list)", "sum list elements"},
    {Function::len, "len", 1, 1, BuiltinFunctionCategory::list, false, "len(list)", "list length"},
    {Function::product, "product", 1, 1, BuiltinFunctionCategory::list, false, "product(list)", "product of list elements"},
    {Function::avg, "avg", 1, 1, BuiltinFunctionCategory::list, false, "avg(list)", "average of list elements"},
    {Function::min, "min", 1, 1, BuiltinFunctionCategory::list, false, "min(list)", "minimum list element"},
    {Function::max, "max", 1, 1, BuiltinFunctionCategory::list, false, "max(list)", "maximum list element"},
    {Function::first, "first", 2, 2, BuiltinFunctionCategory::list, false, "first(n, list)", "first n list elements"},
    {Function::drop, "drop", 2, 2, BuiltinFunctionCategory::list, false, "drop(n, list)", "drop first n list elements"},
    {Function::list_add, "list_add", 2, 2, BuiltinFunctionCategory::list, false,
     "list_add(a, b)", "add matching list elements"},
    {Function::list_sub, "list_sub", 2, 2, BuiltinFunctionCategory::list, false,
     "list_sub(a, b)", "subtract matching list elements"},
    {Function::list_div, "list_div", 2, 2, BuiltinFunctionCategory::list, false,
     "list_div(a, b)", "divide matching list elements"},
    {Function::list_mul, "list_mul", 2, 2, BuiltinFunctionCategory::list, false,
     "list_mul(a, b)", "multiply matching list elements"},
    {Function::guard, "guard", 2, 2, BuiltinFunctionCategory::scalar, false,
     "guard(expr, fallback)", "use fallback when expr evaluation fails"},
    {Function::reduce, "reduce", 2, 2, BuiltinFunctionCategory::list, false,
     "reduce(list, op)", "reduce list with binary operator"},
    {Function::map, "map", 2, 2, BuiltinFunctionCategory::list, false, "map(list, expr)", "map inline expression over list"},
    {Function::range, "range", 2, 3, BuiltinFunctionCategory::list_generation, false,
     "range(start, count[, step])", "generate linear series from start"},
    {Function::geom, "geom", 2, 3, BuiltinFunctionCategory::list_generation, false,
     "geom(start, count[, ratio])", "generate geometric series from start"},
    {Function::repeat, "repeat", 2, 2, BuiltinFunctionCategory::list_generation, false,
     "repeat(value, count)", "repeat value count times"},
    {Function::linspace, "linspace", 3, 3, BuiltinFunctionCategory::list_generation, false,
     "linspace(start, stop, count)", "generate evenly spaced values over interval"},
    {Function::powers, "powers", 2, 3, BuiltinFunctionCategory::list_generation, false,
     "powers(base, count[, start_exp])", "generate successive integer powers"},
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

bool builtin_function_accepts_arity(Function function, std::size_t arity) {
    const auto& info = builtin_function_info(function);
    return arity >= info.min_arity && arity <= info.max_arity;
}

std::string builtin_function_arity_label(Function function) {
    const auto& info = builtin_function_info(function);
    if (info.min_arity == info.max_arity) {
        return std::to_string(info.min_arity);
    }
    return std::to_string(info.min_arity) + "-" + std::to_string(info.max_arity);
}

std::string_view builtin_function_signature(Function function) {
    return builtin_function_info(function).signature;
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
    return builtin_function_info(function).category != BuiltinFunctionCategory::scalar;
}

bool is_unary_scalar_function(Function function) {
    const auto& info = builtin_function_info(function);
    return info.category == BuiltinFunctionCategory::scalar && info.min_arity == 1 &&
           info.max_arity == 1;
}

bool is_mappable_unary_scalar_function(Function function) {
    const auto& info = builtin_function_info(function);
    return info.category == BuiltinFunctionCategory::scalar && info.min_arity == 1 &&
           info.max_arity == 1 && info.mappable;
}

}  // namespace console_calc
