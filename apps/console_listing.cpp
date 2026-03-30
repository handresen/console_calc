#include "console_listing.h"

#include <algorithm>
#include <string>
#include <vector>

#include "console_calc/value_format.h"

namespace console_calc {

namespace {

constexpr std::size_t k_max_console_list_entries = 10;

std::string category_heading(BuiltinFunctionCategory category) {
    switch (category) {
    case BuiltinFunctionCategory::scalar:
        return "Scalar functions";
    case BuiltinFunctionCategory::position:
        return "Position functions";
    case BuiltinFunctionCategory::list:
        return "List functions";
    case BuiltinFunctionCategory::statistics:
        return "Statistics functions";
    case BuiltinFunctionCategory::list_generation:
        return "List generation functions";
    }

    return "Functions";
}

std::string format_console_list(const ListValue& values, IntegerDisplayMode mode) {
    std::string output = "{";
    const std::size_t shown = std::min(values.size(), k_max_console_list_entries);
    for (std::size_t index = 0; index < shown; ++index) {
        if (index != 0) {
            output += ", ";
        }
        output += format_scalar(values[index], mode);
    }

    if (values.size() > k_max_console_list_entries) {
        if (shown != 0) {
            output += ", ";
        }
        output += "<hiding ";
        output += std::to_string(values.size() - k_max_console_list_entries);
        output += " entries>";
    }

    output += '}';
    return output;
}

std::string format_console_position_list(const PositionListValue& values) {
    std::string output = "{";
    const std::size_t shown = std::min(values.size(), k_max_console_list_entries);
    for (std::size_t index = 0; index < shown; ++index) {
        if (index != 0) {
            output += ", ";
        }
        output += format_position(values[index]);
    }

    if (values.size() > k_max_console_list_entries) {
        if (shown != 0) {
            output += ", ";
        }
        output += "<hiding ";
        output += std::to_string(values.size() - k_max_console_list_entries);
        output += " entries>";
    }

    output += '}';
    return output;
}

std::string format_definition_display_name(std::string_view name, const UserDefinition& definition) {
    if (!is_value_definition(definition)) {
        const auto& function = as_function_definition(definition);
        std::string display_name(name);
        display_name += '(';
        for (std::size_t index = 0; index < function.parameters.size(); ++index) {
            if (index != 0) {
                display_name += ", ";
            }
            display_name += function.parameters[index];
        }
        display_name += ')';
        return display_name;
    }
    return std::string(name);
}

std::string_view constant_namespace(std::string_view name) {
    const std::size_t separator = name.find('.');
    if (separator == std::string_view::npos) {
        return "root";
    }
    return name.substr(0, separator);
}

int constant_namespace_order(std::string_view name) {
    const std::string_view group = constant_namespace(name);
    if (group == "root") {
        return 0;
    }
    if (group == "m") {
        return 1;
    }
    if (group == "c") {
        return 2;
    }
    if (group == "ph") {
        return 3;
    }
    return 4;
}

}  // namespace

std::string format_stack_listing(std::span<const Value> values) {
    return format_stack_listing(values, IntegerDisplayMode::decimal);
}

std::vector<StackEntryView> stack_entry_views(std::span<const Value> values) {
    std::vector<StackEntryView> entries;
    entries.reserve(values.size());
    for (std::size_t index = 0; index < values.size(); ++index) {
        entries.push_back(StackEntryView{
            .level = index,
            .value = values[index],
        });
    }
    return entries;
}

std::string format_stack_listing(std::span<const Value> values, IntegerDisplayMode mode) {
    return format_stack_listing(stack_entry_views(values), mode);
}

std::string format_stack_listing(std::span<const StackEntryView> entries, IntegerDisplayMode mode) {
    std::string output;
    for (const auto& entry : entries) {
        output += std::to_string(entry.level);
        output += ':';
        output += format_console_value(entry.value, mode);
        output += '\n';
    }

    return output;
}

std::string format_console_value(const Value& value, IntegerDisplayMode mode) {
    if (const auto* list = std::get_if<ListValue>(&value)) {
        return format_console_list(*list, mode);
    }
    if (const auto* positions = std::get_if<PositionListValue>(&value)) {
        return format_console_position_list(*positions);
    }

    return format_value(value, mode);
}

std::vector<DefinitionView> definition_views(const DefinitionTable& definitions) {
    std::vector<DefinitionView> views;
    views.reserve(definitions.size());
    for (const auto& [name, definition] : definitions) {
        views.push_back(DefinitionView{
            .name = format_definition_display_name(name, definition),
            .expression = is_value_definition(definition)
                              ? as_value_definition(definition).expression
                              : as_function_definition(definition).expression,
        });
    }

    std::sort(views.begin(), views.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.name < rhs.name;
    });
    return views;
}

std::string format_definition_listing(const DefinitionTable& definitions) {
    return format_definition_listing(definition_views(definitions));
}

std::string format_definition_listing(std::span<const DefinitionView> definitions) {
    std::string output;
    for (const auto& view : definitions) {
        output += view.name;
        output += ':';
        output += view.expression;
        output += '\n';
    }
    return output;
}

std::vector<ConstantView> constant_views(const ConstantTable& constants) {
    std::vector<ConstantView> views;
    views.reserve(constants.size());
    for (const auto& [name, value] : constants) {
        views.push_back(ConstantView{
            .name = name,
            .value = value,
        });
    }

    std::sort(views.begin(), views.end(), [](const auto& lhs, const auto& rhs) {
        const int lhs_order = constant_namespace_order(lhs.name);
        const int rhs_order = constant_namespace_order(rhs.name);
        if (lhs_order != rhs_order) {
            return lhs_order < rhs_order;
        }
        return lhs.name < rhs.name;
    });
    return views;
}

std::string format_constant_listing(const ConstantTable& constants) {
    return format_constant_listing(constant_views(constants));
}

std::string format_constant_listing(std::span<const ConstantView> constants) {
    std::string output;
    std::string_view current_namespace;
    bool first_group = true;
    for (const auto& view : constants) {
        const std::string_view view_namespace = constant_namespace(view.name);
        if (view_namespace != current_namespace) {
            if (!first_group) {
                output += '\n';
            }
            output += '[';
            output += view_namespace;
            output += "]\n";
            current_namespace = view_namespace;
            first_group = false;
        }

        output += "  ";
        output += view.name;
        output += ':';
        output += format_scalar(view.value);
        output += '\n';
    }
    return output;
}

std::vector<FunctionView> builtin_function_views(std::span<const BuiltinFunctionInfo> functions) {
    std::vector<FunctionView> views;
    views.reserve(functions.size());
    for (const auto& function : functions) {
        views.push_back(FunctionView{
            .name = std::string(function.name),
            .signature = std::string(builtin_function_signature(function.function)),
            .category = function.category,
            .summary = std::string(function.summary),
        });
    }

    std::sort(views.begin(), views.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.category != rhs.category) {
            return static_cast<int>(lhs.category) < static_cast<int>(rhs.category);
        }
        return lhs.name < rhs.name;
    });
    return views;
}

std::vector<FunctionView> function_views(std::span<const BuiltinFunctionInfo> functions,
                                         std::span<const SpecialFormInfo> special_forms) {
    std::vector<FunctionView> views = builtin_function_views(functions);
    views.reserve(views.size() + special_forms.size());
    for (const auto& form : special_forms) {
        views.push_back(FunctionView{
            .name = std::string(form.name),
            .signature = std::string(form.signature),
            .category = form.category,
            .summary = std::string(form.summary),
        });
    }

    std::sort(views.begin(), views.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.category != rhs.category) {
            return static_cast<int>(lhs.category) < static_cast<int>(rhs.category);
        }
        return lhs.name < rhs.name;
    });
    return views;
}

std::string format_builtin_function_listing(std::span<const BuiltinFunctionInfo> functions) {
    return format_builtin_function_listing(builtin_function_views(functions));
}

std::string format_function_listing(std::span<const BuiltinFunctionInfo> functions,
                                    std::span<const SpecialFormInfo> special_forms) {
    return format_builtin_function_listing(function_views(functions, special_forms));
}

std::string format_builtin_function_listing(std::span<const FunctionView> views) {
    std::vector<FunctionView> scalar_entries;
    std::vector<FunctionView> position_entries;
    std::vector<FunctionView> list_entries;
    std::vector<FunctionView> statistics_entries;
    std::vector<FunctionView> list_generation_entries;
    scalar_entries.reserve(views.size());
    position_entries.reserve(views.size());
    list_entries.reserve(views.size());
    statistics_entries.reserve(views.size());
    list_generation_entries.reserve(views.size());
    std::size_t label_width = 0;

    for (const auto& function : views) {
        const std::string& label = function.signature;
        label_width = std::max(label_width, label.size());
        if (function.category == BuiltinFunctionCategory::position) {
            position_entries.push_back(function);
        } else if (function.category == BuiltinFunctionCategory::list) {
            list_entries.push_back(function);
        } else if (function.category == BuiltinFunctionCategory::statistics) {
            statistics_entries.push_back(function);
        } else if (function.category == BuiltinFunctionCategory::list_generation) {
            list_generation_entries.push_back(function);
        } else {
            scalar_entries.push_back(function);
        }
    }

    auto append_entries = [&](std::string& output, const auto& entries) {
        for (const auto& entry : entries) {
            const std::string& label = entry.signature;
            output += "  ";
            output += label;
            output.append(label_width - label.size(), ' ');
            output += "  ";
            output += entry.summary;
            output += '\n';
        }
    };

    std::string output = category_heading(BuiltinFunctionCategory::scalar) + '\n';
    append_entries(output, scalar_entries);
    output += '\n';
    output += category_heading(BuiltinFunctionCategory::position);
    output += '\n';
    append_entries(output, position_entries);
    output += '\n';
    output += category_heading(BuiltinFunctionCategory::list);
    output += '\n';
    append_entries(output, list_entries);
    output += '\n';
    output += category_heading(BuiltinFunctionCategory::statistics);
    output += '\n';
    append_entries(output, statistics_entries);
    output += '\n';
    output += category_heading(BuiltinFunctionCategory::list_generation);
    output += '\n';
    append_entries(output, list_generation_entries);

    return output;
}

}  // namespace console_calc
