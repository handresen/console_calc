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

void append_stack_json(std::ostringstream& json, std::span<const BindingStackEntry> stack) {
    json << '[';
    for (std::size_t index = 0; index < stack.size(); ++index) {
        if (index != 0) {
            json << ',';
        }
        const auto& entry = stack[index];
        json << "{\"level\":" << entry.level << ",\"display\":\"" << json_escape(entry.display)
             << "\"}";
    }
    json << ']';
}

void append_definitions_json(std::ostringstream& json,
                             std::span<const BindingDefinitionEntry> definitions) {
    json << '[';
    for (std::size_t index = 0; index < definitions.size(); ++index) {
        if (index != 0) {
            json << ',';
        }
        const auto& definition = definitions[index];
        json << "{\"name\":\"" << json_escape(definition.name) << "\",\"expression\":\""
             << json_escape(definition.expression) << "\"}";
    }
    json << ']';
}

void append_constants_json(std::ostringstream& json,
                           std::span<const BindingConstantEntry> constants) {
    json << '[';
    for (std::size_t index = 0; index < constants.size(); ++index) {
        if (index != 0) {
            json << ',';
        }
        const auto& constant = constants[index];
        json << "{\"name\":\"" << json_escape(constant.name) << "\",\"value\":\""
             << json_escape(constant.value) << "\"}";
    }
    json << ']';
}

void append_functions_json(std::ostringstream& json,
                           std::span<const BindingFunctionEntry> functions) {
    json << '[';
    for (std::size_t index = 0; index < functions.size(); ++index) {
        if (index != 0) {
            json << ',';
        }
        const auto& function = functions[index];
        json << "{\"name\":\"" << json_escape(function.name) << "\",\"arity_label\":\""
             << json_escape(function.arity_label) << "\",\"category\":\""
             << json_escape(function.category) << "\",\"summary\":\""
             << json_escape(function.summary) << "\"}";
    }
    json << ']';
}

void append_snapshot_json(std::ostringstream& json, const BindingSnapshot& snapshot) {
    json << "{\"display_mode\":\"" << json_escape(snapshot.display_mode) << '"';
    json << "\",\"max_stack_depth\":" << snapshot.max_stack_depth;
    json << ",\"stack\":";
    append_stack_json(json, snapshot.stack);
    json << ",\"definitions\":";
    append_definitions_json(json, snapshot.definitions);
    json << ",\"constants\":";
    append_constants_json(json, snapshot.constants);
    json << ",\"functions\":";
    append_functions_json(json, snapshot.functions);
    json << '}';
}

void append_event_json(std::ostringstream& json, const BindingEvent& event) {
    json << "{\"kind\":\"" << event_kind_name(event.kind) << '"';
    json << "\",\"text\":\"" << json_escape(event.text) << '"';
    json << "\",\"stack\":";
    append_stack_json(json, event.stack);
    json << ",\"definitions\":";
    append_definitions_json(json, event.definitions);
    json << ",\"constants\":";
    append_constants_json(json, event.constants);
    json << ",\"functions\":";
    append_functions_json(json, event.functions);
    json << '}';
}

std::string result_to_json(const BindingCommandResult& result) {
    std::ostringstream json;
    json << "{\"should_exit\":" << (result.should_exit ? "true" : "false");
    json << "\",\"events\":[";
    for (std::size_t index = 0; index < result.events.size(); ++index) {
        if (index != 0) {
            json << ',';
        }
        append_event_json(json, result.events[index]);
    }
    json << "],\"snapshot\":";
    append_snapshot_json(json, result.snapshot);
    json << '}';
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
    session->last_result_json =
        console_calc::result_to_json({.should_exit = false, .snapshot = session->facade.snapshot()});
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
