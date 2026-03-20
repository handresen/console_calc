#include "console_calc/special_form.h"

#include <array>
#include <stdexcept>

namespace console_calc {

namespace {

constexpr std::array<SpecialFormInfo, 5> k_special_forms = {{
    {Function::guard, "guard", 2, 2, BuiltinFunctionCategory::scalar, "guard(expr, fallback)",
     "use fallback when expr evaluation fails"},
    {Function::reduce, "reduce", 2, 2, BuiltinFunctionCategory::list, "reduce(list, op)",
     "reduce list with binary operator"},
    {Function::timed_loop, "timed_loop", 2, 2, BuiltinFunctionCategory::scalar,
     "timed_loop(expr, count)", "evaluate expr count times and return elapsed seconds"},
    {Function::fill, "fill", 2, 2, BuiltinFunctionCategory::list_generation, "fill(expr, count)",
     "evaluate expr count times into a list"},
    {Function::map, "map", 2, 5, BuiltinFunctionCategory::list,
     "map(list, expr[, start[, step[, count]]])",
     "map inline expression over list slice"},
}};

}  // namespace

std::optional<Function> parse_special_form_function(std::string_view name) {
    for (const auto& form : k_special_forms) {
        if (form.name == name) {
            return form.function;
        }
    }
    return std::nullopt;
}

bool is_special_form_name(std::string_view name) {
    return parse_special_form_function(name).has_value();
}

bool is_special_form(Function function) {
    return function == Function::guard || function == Function::reduce ||
           function == Function::timed_loop || function == Function::fill ||
           function == Function::map;
}

const SpecialFormInfo& special_form_info(Function function) {
    for (const auto& form : k_special_forms) {
        if (form.function == function) {
            return form;
        }
    }

    throw std::invalid_argument("unknown special form");
}

std::span<const SpecialFormInfo> special_forms() { return k_special_forms; }

bool special_form_accepts_arity(Function function, std::size_t arity) {
    const auto& info = special_form_info(function);
    return arity >= info.min_arity && arity <= info.max_arity;
}

std::string_view special_form_signature(Function function) {
    return special_form_info(function).signature;
}

bool is_list_generation_special_form(Function function) {
    return is_special_form(function) &&
           special_form_info(function).category == BuiltinFunctionCategory::list_generation;
}

}  // namespace console_calc
