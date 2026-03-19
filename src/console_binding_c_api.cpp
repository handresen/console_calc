#include "console_calc/console_binding_c_api.h"

#include <memory>
#include <sstream>
#include <string>

#include "compile_time_constants.h"
#include "console_calc/console_binding_facade.h"
#include "console_calc/expression_parser.h"

struct console_calc_binding_session {
    console_calc::ExpressionParser parser;
    console_calc::ConsoleBindingFacade facade{parser, console_calc::builtin_constant_table()};
    std::string last_result_json = "{}";
};

namespace console_calc {

namespace {

std::string json_escape(std::string_view input) {
    std::string output;
    output.reserve(input.size());
    for (const char ch : input) {
        switch (ch) {
        case '\\':
            output += "\\\\";
            break;
        case '"':
            output += "\\\"";
            break;
        case '\n':
            output += "\\n";
            break;
        case '\r':
            output += "\\r";
            break;
        case '\t':
            output += "\\t";
            break;
        default:
            output.push_back(ch);
            break;
        }
    }
    return output;
}

std::string event_kind_name(BindingEventKind kind) {
    switch (kind) {
    case BindingEventKind::value:
        return "value";
    case BindingEventKind::text:
        return "text";
    case BindingEventKind::stack_listing:
        return "stack_listing";
    case BindingEventKind::definition_listing:
        return "definition_listing";
    case BindingEventKind::constant_listing:
        return "constant_listing";
    case BindingEventKind::function_listing:
        return "function_listing";
    case BindingEventKind::error:
        return "error";
    }

    return "text";
}

std::string result_to_json(const BindingCommandResult& result) {
    std::ostringstream json;
    json << "{\"should_exit\":" << (result.should_exit ? "true" : "false");
    json << ",\"display_mode\":\"" << json_escape(result.snapshot.display_mode) << '"';
    json << "\",\"events\":[";
    for (std::size_t index = 0; index < result.events.size(); ++index) {
        if (index != 0) {
            json << ',';
        }
        json << "{\"kind\":\"" << event_kind_name(result.events[index].kind) << '"';
        json << ",\"text\":\"" << json_escape(result.events[index].text) << "\"}";
    }
    json << "],\"stack\":[";
    for (std::size_t index = 0; index < result.snapshot.stack.size(); ++index) {
        if (index != 0) {
            json << ',';
        }
        const auto& entry = result.snapshot.stack[index];
        json << "{\"level\":" << entry.level
             << ",\"display\":\"" << json_escape(entry.display) << "\"}";
    }
    json << "]}";
    return json.str();
}

}  // namespace

}  // namespace console_calc

extern "C" {

console_calc_binding_session* console_calc_binding_session_create(void) {
    return new console_calc_binding_session();
}

void console_calc_binding_session_destroy(console_calc_binding_session* session) {
    delete session;
}

int console_calc_binding_session_initialize(console_calc_binding_session* session) {
    if (session == nullptr) {
        return 1;
    }

    session->facade.initialize();
    return 0;
}

int console_calc_binding_session_submit(console_calc_binding_session* session, const char* input) {
    if (session == nullptr || input == nullptr) {
        return 1;
    }

    const auto result = session->facade.submit(input);
    session->last_result_json = console_calc::result_to_json(result);
    return 0;
}

const char* console_calc_binding_session_last_result_json(
    const console_calc_binding_session* session) {
    if (session == nullptr) {
        return "{}";
    }

    return session->last_result_json.c_str();
}

}  // extern "C"
