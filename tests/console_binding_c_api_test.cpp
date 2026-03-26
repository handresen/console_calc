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
        expect_contains(result_json, "\"error\":{\"message\":\"function 'guard' expects guard(expr, fallback)\"",
                        "structured error message") &&
        expect_contains(result_json, "\"expected_signature\":\"guard(expr, fallback)\"",
                        "structured expected signature") &&
        expect_contains(result_json, "function 'guard' expects guard(expr, fallback)",
                        "guard signature error");

    cleanup();
    return ok;
}

bool expect_c_api_multilist_payloads() {
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

    if (console_calc_binding_session_submit(session, "{{1,2},{3,4}}") != 0) {
        std::cerr << "Failed to submit multi-list expression to binding session\n";
        cleanup();
        return false;
    }

    const std::string_view scalar_json = console_calc_binding_session_last_result_json(session);
    const bool scalar_ok =
        expect_contains(scalar_json, "\"multi_list_values\":[[1,2],[3,4]]",
                        "scalar multi-list payload") &&
        expect_contains(scalar_json, "\"multi_position_list_values\":[]",
                        "empty multi-position-list payload");

    if (!scalar_ok) {
        cleanup();
        return false;
    }

    if (console_calc_binding_session_submit(session, "{{pos(0,0),pos(0,1)},{pos(1,1)}}") != 0) {
        std::cerr << "Failed to submit multi position list expression to binding session\n";
        cleanup();
        return false;
    }

    const std::string_view position_json = console_calc_binding_session_last_result_json(session);
    const bool position_ok = expect_contains(
        position_json,
        "\"multi_position_list_values\":[[{\"latitude_deg\":0,\"longitude_deg\":0},{\"latitude_deg\":0,\"longitude_deg\":1}],[{\"latitude_deg\":1,\"longitude_deg\":1}]]",
        "multi-position-list payload");

    cleanup();
    return position_ok;
}

bool expect_c_api_input_validation() {
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

    const bool ok =
        console_calc_binding_session_is_valid_input(session, "") != 0 &&
        console_calc_binding_session_is_valid_input(session, "1+1") != 0 &&
        console_calc_binding_session_is_valid_input(session, "vars") != 0 &&
        console_calc_binding_session_is_valid_input(session, "guard()") == 0 &&
        console_calc_binding_session_is_valid_input(session, "#f(x):x+1") == 0;

    cleanup();
    return ok;
}

}  // namespace

int main() {
    return expect_c_api_round_trip() && expect_c_api_invalid_input_error_result() &&
                   expect_c_api_multilist_payloads() && expect_c_api_input_validation()
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
