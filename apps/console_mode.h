#pragma once

#include <iosfwd>

namespace console_calc {

class ExpressionParser;

int run_console_mode(const ExpressionParser& parser, std::istream& input, std::ostream& output,
                     std::ostream& error);

}  // namespace console_calc
