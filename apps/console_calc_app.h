#pragma once

#include <iosfwd>
#include <span>
#include <string_view>

namespace console_calc {

int run_console_calc(std::span<const std::string_view> args, std::istream& input,
                     std::ostream& output, std::ostream& error);

}  // namespace console_calc
