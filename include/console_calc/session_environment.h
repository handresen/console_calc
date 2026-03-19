#pragma once

#include <string>
#include <unordered_map>

namespace console_calc {

using ConstantTable = std::unordered_map<std::string, double>;

struct UserDefinition {
    std::string expression;
};

using DefinitionTable = std::unordered_map<std::string, UserDefinition>;

}  // namespace console_calc
