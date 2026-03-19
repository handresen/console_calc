#pragma once

#include <cstdio>
#include <string>
#include <string_view>

namespace console_calc::test {

constexpr std::string_view k_green_prompt = "\x1b[32m";
constexpr std::string_view k_color_reset = "\x1b[0m";

[[nodiscard]] inline std::string prompt(std::size_t depth) {
    return std::string(k_green_prompt) + std::to_string(depth) + '>' +
           std::string(k_color_reset);
}

[[nodiscard]] inline bool expect_text_eq(std::string_view label, const std::string& actual,
                                         const std::string& expected) {
    if (actual == expected) {
        return true;
    }

    std::fprintf(stderr, "%.*s mismatch\n", static_cast<int>(label.size()), label.data());
    std::fprintf(stderr, "expected: [%s]\n", expected.c_str());
    std::fprintf(stderr, "actual:   [%s]\n", actual.c_str());
    return false;
}

[[nodiscard]] inline bool expect_console_transcript(std::string_view label, int actual_exit_code,
                                                    int expected_exit_code,
                                                    const std::string& actual_output,
                                                    const std::string& expected_output,
                                                    const std::string& actual_error,
                                                    const std::string& expected_error) {
    bool matches = true;
    if (actual_exit_code != expected_exit_code) {
        std::fprintf(stderr, "%.*s exit code mismatch\n", static_cast<int>(label.size()),
                     label.data());
        std::fprintf(stderr, "expected: [%d]\n", expected_exit_code);
        std::fprintf(stderr, "actual:   [%d]\n", actual_exit_code);
        matches = false;
    }

    if (!expect_text_eq(std::string(label) + " output", actual_output, expected_output)) {
        matches = false;
    }
    if (!expect_text_eq(std::string(label) + " error", actual_error, expected_error)) {
        matches = false;
    }

    return matches;
}

}  // namespace console_calc::test
