#pragma once

#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "expression_environment.h"
#include "console_session_engine.h"

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

[[nodiscard]] inline bool definitions_equal(const DefinitionTable& lhs, const DefinitionTable& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (const auto& [name, definition] : lhs) {
        const auto found = rhs.find(name);
        if (found == rhs.end() || found->second.expression != definition.expression) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] inline bool expect_single_value_event(std::string_view label,
                                                    const ConsoleEngineCommandResult& result) {
    if (result.events.size() == 1 &&
        result.events[0].kind == ConsoleOutputEventKind::value &&
        result.events[0].value.has_value()) {
        return true;
    }

    std::fprintf(stderr, "%.*s expected one value event, got %zu events\n",
                 static_cast<int>(label.size()), label.data(), result.events.size());
    return false;
}

[[nodiscard]] inline bool expect_single_text_event(std::string_view label,
                                                   const ConsoleEngineCommandResult& result,
                                                   std::string_view expected_text) {
    if (result.events.size() == 1 &&
        result.events[0].kind == ConsoleOutputEventKind::text &&
        result.events[0].text == expected_text) {
        return true;
    }

    std::fprintf(stderr, "%.*s text event mismatch\n", static_cast<int>(label.size()),
                 label.data());
    std::fprintf(stderr, "expected: [%.*s]\n", static_cast<int>(expected_text.size()),
                 expected_text.data());
    if (result.events.size() == 1) {
        std::fprintf(stderr, "actual:   [%s]\n", result.events[0].text.c_str());
    } else {
        std::fprintf(stderr, "actual:   [%zu events]\n", result.events.size());
    }
    return false;
}

[[nodiscard]] inline bool expect_single_error_event(std::string_view label,
                                                    const ConsoleEngineCommandResult& result,
                                                    std::string_view expected_text) {
    if (result.events.size() == 1 &&
        result.events[0].kind == ConsoleOutputEventKind::error &&
        result.events[0].text == expected_text) {
        return true;
    }

    std::fprintf(stderr, "%.*s error event mismatch\n", static_cast<int>(label.size()),
                 label.data());
    std::fprintf(stderr, "expected: [%.*s]\n", static_cast<int>(expected_text.size()),
                 expected_text.data());
    if (result.events.size() == 1) {
        std::fprintf(stderr, "actual:   [%s]\n", result.events[0].text.c_str());
    } else {
        std::fprintf(stderr, "actual:   [%zu events]\n", result.events.size());
    }
    return false;
}

[[nodiscard]] inline bool expect_single_stack_listing_event(
    std::string_view label, const ConsoleEngineCommandResult& result) {
    if (result.events.size() == 1 &&
        result.events[0].kind == ConsoleOutputEventKind::stack_listing) {
        return true;
    }

    std::fprintf(stderr, "%.*s expected one stack listing event, got %zu events\n",
                 static_cast<int>(label.size()), label.data(), result.events.size());
    return false;
}

[[nodiscard]] inline bool expect_single_definition_listing_event(
    std::string_view label, const ConsoleEngineCommandResult& result) {
    if (result.events.size() == 1 &&
        result.events[0].kind == ConsoleOutputEventKind::definition_listing) {
        return true;
    }

    std::fprintf(stderr, "%.*s expected one definition listing event, got %zu events\n",
                 static_cast<int>(label.size()), label.data(), result.events.size());
    return false;
}

[[nodiscard]] inline bool expect_single_constant_listing_event(
    std::string_view label, const ConsoleEngineCommandResult& result) {
    if (result.events.size() == 1 &&
        result.events[0].kind == ConsoleOutputEventKind::constant_listing) {
        return true;
    }

    std::fprintf(stderr, "%.*s expected one constant listing event, got %zu events\n",
                 static_cast<int>(label.size()), label.data(), result.events.size());
    return false;
}

[[nodiscard]] inline bool expect_single_function_listing_event(
    std::string_view label, const ConsoleEngineCommandResult& result) {
    if (result.events.size() == 1 &&
        result.events[0].kind == ConsoleOutputEventKind::function_listing) {
        return true;
    }

    std::fprintf(stderr, "%.*s expected one function listing event, got %zu events\n",
                 static_cast<int>(label.size()), label.data(), result.events.size());
    return false;
}

}  // namespace console_calc::test
