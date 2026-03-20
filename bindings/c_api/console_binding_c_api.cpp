#include "console_calc/console_binding_c_api.h"

#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

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

class JsonWriter {
public:
    void begin_object() { begin_container('{'); }
    void end_object() { end_container('}'); }
    void begin_array() { begin_container('['); }
    void end_array() { end_container(']'); }

    void key(std::string_view name) {
        prefix_value();
        write_string(name);
        stream_ << ':';
        expecting_value_ = true;
    }

    void value(std::string_view input) {
        prefix_value();
        write_string(input);
    }

    void value(bool input) {
        prefix_value();
        stream_ << (input ? "true" : "false");
    }

    template <typename Number>
    void value(Number input) requires std::is_arithmetic_v<Number>
    {
        prefix_value();
        stream_ << input;
    }

    void null_value() {
        prefix_value();
        stream_ << "null";
    }

    [[nodiscard]] std::string str() const { return stream_.str(); }

private:
    void begin_container(char opener) {
        prefix_value();
        stream_ << opener;
        first_stack_.push_back(true);
    }

    void end_container(char closer) {
        stream_ << closer;
        first_stack_.pop_back();
    }

    void prefix_value() {
        if (expecting_value_) {
            expecting_value_ = false;
            return;
        }
        if (first_stack_.empty()) {
            return;
        }
        if (!first_stack_.back()) {
            stream_ << ',';
        } else {
            first_stack_.back() = false;
        }
    }

    void write_string(std::string_view input) {
        stream_ << '"';
        for (const char ch : input) {
            switch (ch) {
            case '\\':
                stream_ << "\\\\";
                break;
            case '"':
                stream_ << "\\\"";
                break;
            case '\n':
                stream_ << "\\n";
                break;
            case '\r':
                stream_ << "\\r";
                break;
            case '\t':
                stream_ << "\\t";
                break;
            default:
                stream_ << ch;
                break;
            }
        }
        stream_ << '"';
    }

    std::ostringstream stream_;
    std::vector<bool> first_stack_;
    bool expecting_value_ = false;
};

void append_stack_json(JsonWriter& json, std::span<const BindingStackEntry> stack) {
    json.begin_array();
    for (const auto& entry : stack) {
        json.begin_object();
        json.key("level");
        json.value(entry.level);
        json.key("display");
        json.value(entry.display);
        json.key("list_values");
        json.begin_array();
        for (const double value : entry.list_values) {
            json.value(value);
        }
        json.end_array();
        json.key("position");
        if (entry.position.has_value()) {
            json.begin_object();
            json.key("latitude_deg");
            json.value(entry.position->latitude_deg);
            json.key("longitude_deg");
            json.value(entry.position->longitude_deg);
            json.end_object();
        } else {
            json.null_value();
        }
        json.end_object();
    }
    json.end_array();
}

void append_definitions_json(JsonWriter& json,
                             std::span<const BindingDefinitionEntry> definitions) {
    json.begin_array();
    for (const auto& definition : definitions) {
        json.begin_object();
        json.key("name");
        json.value(definition.name);
        json.key("expression");
        json.value(definition.expression);
        json.end_object();
    }
    json.end_array();
}

void append_constants_json(JsonWriter& json,
                           std::span<const BindingConstantEntry> constants) {
    json.begin_array();
    for (const auto& constant : constants) {
        json.begin_object();
        json.key("name");
        json.value(constant.name);
        json.key("value");
        json.value(constant.value);
        json.end_object();
    }
    json.end_array();
}

void append_functions_json(JsonWriter& json,
                           std::span<const BindingFunctionEntry> functions) {
    json.begin_array();
    for (const auto& function : functions) {
        json.begin_object();
        json.key("name");
        json.value(function.name);
        json.key("signature");
        json.value(function.signature);
        json.key("category");
        json.value(function.category);
        json.key("summary");
        json.value(function.summary);
        json.end_object();
    }
    json.end_array();
}

void append_snapshot_json(JsonWriter& json, const BindingSnapshot& snapshot) {
    json.begin_object();
    json.key("display_mode");
    json.value(snapshot.display_mode);
    json.key("max_stack_depth");
    json.value(snapshot.max_stack_depth);
    json.key("stack");
    append_stack_json(json, snapshot.stack);
    json.key("definitions");
    append_definitions_json(json, snapshot.definitions);
    json.key("constants");
    append_constants_json(json, snapshot.constants);
    json.key("functions");
    append_functions_json(json, snapshot.functions);
    json.end_object();
}

void append_event_json(JsonWriter& json, const BindingEvent& event) {
    json.begin_object();
    json.key("kind");
    json.value(event_kind_name(event.kind));
    json.key("text");
    json.value(event.text);
    json.key("error");
    if (event.error.has_value()) {
        json.begin_object();
        json.key("message");
        json.value(event.error->message);
        json.key("expected_signature");
        if (event.error->expected_signature.has_value()) {
            json.value(*event.error->expected_signature);
        } else {
            json.null_value();
        }
        json.end_object();
    } else {
        json.null_value();
    }
    json.key("stack");
    append_stack_json(json, event.stack);
    json.key("definitions");
    append_definitions_json(json, event.definitions);
    json.key("constants");
    append_constants_json(json, event.constants);
    json.key("functions");
    append_functions_json(json, event.functions);
    json.end_object();
}

std::string result_to_json(const BindingCommandResult& result) {
    JsonWriter json;
    json.begin_object();
    json.key("should_exit");
    json.value(result.should_exit);
    json.key("events");
    json.begin_array();
    for (const auto& event : result.events) {
        append_event_json(json, event);
    }
    json.end_array();
    json.key("snapshot");
    append_snapshot_json(json, result.snapshot);
    json.end_object();
    return json.str();
}

BindingCommandResult error_result_for_session(const console_calc_binding_session& session,
                                              std::string_view message) {
    return BindingCommandResult{
        .should_exit = false,
        .events =
            {BindingEvent{
                .kind = BindingEventKind::error,
                .text = std::string(message),
                .error = ErrorInfo{.message = std::string(message)},
            }},
        .snapshot = session.facade.snapshot(),
    };
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

    try {
        session->facade.initialize();
        session->last_result_json = console_calc::result_to_json(
            {.should_exit = false, .snapshot = session->facade.snapshot()});
    } catch (const std::exception& error) {
        session->last_result_json = console_calc::result_to_json(
            console_calc::error_result_for_session(*session, error.what()));
    } catch (...) {
        session->last_result_json = console_calc::result_to_json(
            console_calc::error_result_for_session(*session, "unknown binding failure"));
    }
    return 0;
}

int console_calc_binding_session_submit(console_calc_binding_session* session, const char* input) {
    if (session == nullptr || input == nullptr) {
        return 1;
    }

    try {
        const auto result = session->facade.submit(input);
        session->last_result_json = console_calc::result_to_json(result);
    } catch (const std::exception& error) {
        session->last_result_json = console_calc::result_to_json(
            console_calc::error_result_for_session(*session, error.what()));
    } catch (...) {
        session->last_result_json = console_calc::result_to_json(
            console_calc::error_result_for_session(*session, "unknown binding failure"));
    }
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
