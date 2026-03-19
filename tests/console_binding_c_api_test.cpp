#include <cstdlib>
#include <iostream>
#include <string_view>

#include "console_calc/console_binding_c_api.h"

namespace {

bool expect_contains(std::string_view text, std::string_view expected, std::string_view label) {
    if (text.find(expected) != std::string_view::npos) {
        return true;
    }

    std::cerr << "Missing expected fragment for " << label << "\nExpected: " << expected
              << "\nActual: " << text << '\n';
    return false;
}

bool expect_c_api_round_trip() {
    console_calc_binding_session* const session = console_calc_binding_session_create();
    if (session == nullptr) {
        std::cerr << "Failed to create binding session\n";
        return false;
    }

    const auto cleanup = [&]() { console_calc_binding_session_destroy(session); };

    if (console_calc_binding_session_initialize(session) != 0) {
        std::cerr << "Failed to initialize binding session\n";
        cleanup();
        return false;
    }

    if (console_calc_binding_session_submit(session, "1+1") != 0) {
        std::cerr << "Failed to submit expression to binding session\n";
        cleanup();
        return false;
    }

    const std::string_view result_json = console_calc_binding_session_last_result_json(session);
    const bool ok = expect_contains(result_json, "\"should_exit\":false", "exit state") &&
                    expect_contains(result_json, "\"snapshot\":{\"display_mode\":\"dec\"",
                                    "snapshot display mode") &&
                    expect_contains(result_json, "\"kind\":\"value\"", "event kind") &&
                    expect_contains(result_json, "\"text\":\"2\"", "event text") &&
                    expect_contains(result_json, "\"display\":\"2\"", "stack display") &&
                    expect_contains(result_json, "\"constants\":[", "constants payload") &&
                    expect_contains(result_json, "\"functions\":[", "functions payload");

    cleanup();
    return ok;
}

bool expect_c_api_invalid_input_error_result() {
    console_calc_binding_session* const session = console_calc_binding_session_create();
    if (session == nullptr) {
        std::cerr << "Failed to create binding session\n";
        return false;
    }

    const auto cleanup = [&]() { console_calc_binding_session_destroy(session); };

    if (console_calc_binding_session_initialize(session) != 0) {
        std::cerr << "Failed to initialize binding session\n";
        cleanup();
        return false;
    }

    if (console_calc_binding_session_submit(session, "guard()") != 0) {
        std::cerr << "Failed to submit invalid expression to binding session\n";
        cleanup();
        return false;
    }

    const std::string_view result_json = console_calc_binding_session_last_result_json(session);
    const bool ok =
        expect_contains(result_json, "\"kind\":\"error\"", "error event kind") &&
        expect_contains(result_json, "function 'guard' expects guard(expr, fallback)",
                        "guard signature error");

    cleanup();
    return ok;
}

}  // namespace

int main() {
    return expect_c_api_round_trip() && expect_c_api_invalid_input_error_result()
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
