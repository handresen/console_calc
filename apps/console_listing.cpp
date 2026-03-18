#include "console_listing.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "console_calc/value_format.h"

namespace console_calc {

namespace {

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
    std::string output;
    for (std::size_t index = 0; index < values.size(); ++index) {
        output += std::to_string(index);
        output += ':';
        output += format_value(values[index]);
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
    std::vector<std::pair<std::string, std::string>> scalar_entries;
    std::vector<std::pair<std::string, std::string>> list_entries;
    scalar_entries.reserve(functions.size());
    list_entries.reserve(functions.size());

    for (const auto& function : functions) {
        std::string line = std::string(function.name) + '/' + std::to_string(function.arity);
        if (function.category == BuiltinFunctionCategory::list) {
            list_entries.emplace_back(std::string(function.name), std::move(line));
        } else {
            scalar_entries.emplace_back(std::string(function.name), std::move(line));
        }
    }

    auto sort_entries = [](auto& entries) {
        std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.first < rhs.first;
        });
    };

    sort_entries(scalar_entries);
    sort_entries(list_entries);

    std::string output = "Scalar functions\n";
    for (const auto& entry : scalar_entries) {
        output += entry.second;
        output += '\n';
    }

    output += "List functions\n";
    for (const auto& entry : list_entries) {
        output += entry.second;
        output += '\n';
    }

    return output;
}

}  // namespace console_calc
