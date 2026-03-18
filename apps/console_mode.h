#pragma once

#include <iosfwd>
#include <string>
#include <unordered_map>

namespace console_calc {

using ConstantTable = std::unordered_map<std::string, double>;
class ExpressionParser;

int run_console_mode(const ExpressionParser& parser, const ConstantTable& constants,
                     std::istream& input, std::ostream& output, std::ostream& error);

}  // namespace console_calc
