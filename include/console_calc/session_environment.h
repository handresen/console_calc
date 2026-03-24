#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace console_calc {

using ConstantTable = std::unordered_map<std::string, double>;

enum class UserDefinitionKind {
    value,
    function,
};

struct UserValueDefinition {
    std::string expression;
};

struct UserFunctionDefinition {
    std::vector<std::string> parameters;
    std::string expression;
};

struct UserDefinition {
    std::variant<UserValueDefinition, UserFunctionDefinition> body;
};

using DefinitionTable = std::unordered_map<std::string, UserDefinition>;

[[nodiscard]] inline UserDefinition make_value_definition(std::string expression) {
    return UserDefinition{
        .body = UserValueDefinition{.expression = std::move(expression)},
    };
}

[[nodiscard]] inline UserDefinition make_function_definition(
    std::vector<std::string> parameters, std::string expression) {
    return UserDefinition{
        .body = UserFunctionDefinition{
            .parameters = std::move(parameters),
            .expression = std::move(expression),
        },
    };
}

[[nodiscard]] inline UserDefinitionKind definition_kind(const UserDefinition& definition) {
    return std::holds_alternative<UserValueDefinition>(definition.body)
               ? UserDefinitionKind::value
               : UserDefinitionKind::function;
}

[[nodiscard]] inline bool is_value_definition(const UserDefinition& definition) {
    return definition_kind(definition) == UserDefinitionKind::value;
}

[[nodiscard]] inline const UserValueDefinition& as_value_definition(
    const UserDefinition& definition) {
    return std::get<UserValueDefinition>(definition.body);
}

[[nodiscard]] inline const UserFunctionDefinition& as_function_definition(
    const UserDefinition& definition) {
    return std::get<UserFunctionDefinition>(definition.body);
}

}  // namespace console_calc
