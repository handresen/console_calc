#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "expression_environment.h"
#include "console_calc/builtin_function.h"
#include "console_calc/scalar_value.h"
#include "console_calc/value.h"
#include "console_calc/value_format.h"

namespace console_calc {

struct StackEntryView {
    std::size_t level = 0;
    Value value;
};

struct DefinitionView {
    std::string name;
    std::string expression;
};

struct ConstantView {
    std::string name;
    ScalarValue value;
};

struct FunctionView {
    std::string name;
    std::string arity_label;
    BuiltinFunctionCategory category = BuiltinFunctionCategory::scalar;
    std::string summary;
};

[[nodiscard]] std::vector<StackEntryView> stack_entry_views(std::span<const Value> values);
[[nodiscard]] std::vector<DefinitionView> definition_views(const DefinitionTable& definitions);
[[nodiscard]] std::vector<ConstantView> constant_views(const ConstantTable& constants);
[[nodiscard]] std::vector<FunctionView> builtin_function_views(
    std::span<const BuiltinFunctionInfo> functions);

[[nodiscard]] std::string format_stack_listing(std::span<const Value> values);
[[nodiscard]] std::string format_stack_listing(std::span<const Value> values,
                                               IntegerDisplayMode mode);
[[nodiscard]] std::string format_console_value(const Value& value, IntegerDisplayMode mode);
[[nodiscard]] std::string format_definition_listing(const DefinitionTable& definitions);
[[nodiscard]] std::string format_constant_listing(const ConstantTable& constants);
[[nodiscard]] std::string format_builtin_function_listing(
    std::span<const BuiltinFunctionInfo> functions);

}  // namespace console_calc
