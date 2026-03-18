#pragma once

#include <string>
#include <string_view>

namespace console_calc::test {

constexpr std::string_view k_green_prompt = "\x1b[32m";
constexpr std::string_view k_color_reset = "\x1b[0m";

[[nodiscard]] inline std::string prompt(std::size_t depth) {
    return std::string(k_green_prompt) + std::to_string(depth) + '>' +
           std::string(k_color_reset);
}

}  // namespace console_calc::test
