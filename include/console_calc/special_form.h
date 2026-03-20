#pragma once

#include <optional>
#include <span>
#include <string_view>

#include "console_calc/builtin_function.h"
#include "console_calc/expression_ast.h"

namespace console_calc {

struct SpecialFormInfo {
    Function function;
    std::string_view name;
    std::size_t min_arity;
    std::size_t max_arity;
    BuiltinFunctionCategory category;
    std::string_view signature;
    std::string_view summary;
};

[[nodiscard]] std::optional<Function> parse_special_form_function(std::string_view name);
[[nodiscard]] bool is_special_form_name(std::string_view name);
[[nodiscard]] bool is_special_form(Function function);
[[nodiscard]] const SpecialFormInfo& special_form_info(Function function);
[[nodiscard]] std::span<const SpecialFormInfo> special_forms();
[[nodiscard]] bool special_form_accepts_arity(Function function, std::size_t arity);
[[nodiscard]] std::string_view special_form_signature(Function function);

}  // namespace console_calc
