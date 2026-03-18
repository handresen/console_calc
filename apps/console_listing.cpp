#include "console_listing.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "console_calc/value_format.h"

namespace console_calc {

namespace {

std::string category_heading(BuiltinFunctionCategory category) {
    switch (category) {
    case BuiltinFunctionCategory::scalar:
        return "Scalar functions";
    case BuiltinFunctionCategory::list:
        return "List functions";
    case BuiltinFunctionCategory::list_generation:
        return "List generation functions";
    }

    return "Functions";
}

template <typename Table, typename Formatter>
std::string format_named_listing(const Table& table, Formatter formatter) {
    std::vector<std::string> names;
    names.reserve(table.size());
    for (const auto& [name, _] : table) {
        names.push_back(name);
    }

    std::sort(names.begin(), names.end());

    std::string output;
    for (const auto& name : names) {
        output += formatter(name);
        output += '\n';
    }

    return output;
}

}  // namespace

std::string format_stack_listing(std::span<const Value> values) {
    return format_stack_listing(values, IntegerDisplayMode::decimal);
}

std::string format_stack_listing(std::span<const Value> values, IntegerDisplayMode mode) {
    std::string output;
    for (std::size_t index = 0; index < values.size(); ++index) {
        output += std::to_string(index);
        output += ':';
        output += format_value(values[index], mode);
        output += '\n';
    }

    return output;
}

std::string format_definition_listing(const DefinitionTable& definitions) {
    return format_named_listing(definitions, [&](const std::string& name) {
        return name + ':' + definitions.at(name).expression;
    });
}

std::string format_constant_listing(const ConstantTable& constants) {
    return format_named_listing(constants, [&](const std::string& name) {
        return name + ':' + format_scalar(constants.at(name));
    });
}

std::string format_builtin_function_listing(std::span<const BuiltinFunctionInfo> functions) {
    std::vector<std::pair<std::string, BuiltinFunctionInfo>> scalar_entries;
    std::vector<std::pair<std::string, BuiltinFunctionInfo>> list_entries;
    std::vector<std::pair<std::string, BuiltinFunctionInfo>> list_generation_entries;
    scalar_entries.reserve(functions.size());
    list_entries.reserve(functions.size());
    list_generation_entries.reserve(functions.size());
    std::size_t label_width = 0;

    for (const auto& function : functions) {
        std::string label = std::string(function.name) + '/' + builtin_function_arity_label(function.function);
        label_width = std::max(label_width, label.size());
        if (function.category == BuiltinFunctionCategory::list) {
            list_entries.emplace_back(std::string(function.name), function);
        } else if (function.category == BuiltinFunctionCategory::list_generation) {
            list_generation_entries.emplace_back(std::string(function.name), function);
        } else {
            scalar_entries.emplace_back(std::string(function.name), function);
        }
    }

    auto sort_entries = [](auto& entries) {
        std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.first < rhs.first;
        });
    };

    sort_entries(scalar_entries);
    sort_entries(list_entries);
    sort_entries(list_generation_entries);

    auto append_entries = [&](std::string& output, const auto& entries) {
        for (const auto& entry : entries) {
            const std::string label =
                std::string(entry.second.name) + '/' + builtin_function_arity_label(entry.second.function);
            output += "  ";
            output += label;
            output.append(label_width - label.size(), ' ');
            output += "  ";
            output += entry.second.summary;
            output += '\n';
        }
    };

    std::string output = category_heading(BuiltinFunctionCategory::scalar) + '\n';
    append_entries(output, scalar_entries);
    output += '\n';
    output += category_heading(BuiltinFunctionCategory::list);
    output += '\n';
    append_entries(output, list_entries);
    output += '\n';
    output += category_heading(BuiltinFunctionCategory::list_generation);
    output += '\n';
    append_entries(output, list_generation_entries);

    return output;
}

}  // namespace console_calc
