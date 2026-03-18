#include "console_command.h"

namespace console_calc {

namespace {

[[nodiscard]] bool is_stack_operator(std::string_view text) {
    return text.size() == 1 &&
           (text[0] == '+' || text[0] == '-' || text[0] == '*' || text[0] == '/' ||
            text[0] == '%' || text[0] == '^' || text[0] == '&' || text[0] == '|');
}

}  // namespace

ConsoleCommand classify_console_command(std::string_view text) {
    if (text == "q" || text == "Q") {
        return {.kind = ConsoleCommandKind::quit};
    }
    if (text == "s") {
        return {.kind = ConsoleCommandKind::list_stack};
    }
    if (text == "vars") {
        return {.kind = ConsoleCommandKind::list_variables};
    }
    if (text == "consts") {
        return {.kind = ConsoleCommandKind::list_constants};
    }
    if (text == "funcs") {
        return {.kind = ConsoleCommandKind::list_functions};
    }
    if (text == "dec") {
        return {.kind = ConsoleCommandKind::display_decimal};
    }
    if (text == "hex") {
        return {.kind = ConsoleCommandKind::display_hexadecimal};
    }
    if (text == "bin") {
        return {.kind = ConsoleCommandKind::display_binary};
    }
    if (text == "dup") {
        return {.kind = ConsoleCommandKind::duplicate};
    }
    if (text == "drop") {
        return {.kind = ConsoleCommandKind::drop};
    }
    if (text == "swap") {
        return {.kind = ConsoleCommandKind::swap};
    }
    if (text == "clear") {
        return {.kind = ConsoleCommandKind::clear};
    }
    if (is_stack_operator(text)) {
        return {.kind = ConsoleCommandKind::stack_operator, .stack_operator = text[0]};
    }
    if (text.find(':') != std::string_view::npos) {
        return {.kind = ConsoleCommandKind::assignment};
    }

    return {.kind = ConsoleCommandKind::expression};
}

}  // namespace console_calc
