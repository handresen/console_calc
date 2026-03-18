#include <cstdlib>
#include <string>

#include "console_history.h"

namespace {

bool expect_console_history_previous() {
    console_calc::ConsoleHistory history;
    history.record("one");
    history.record("two");
    history.record("three");

    const auto first = history.previous();
    const auto second = history.previous();
    const auto third = history.previous();

    return first.has_value() && second.has_value() && third.has_value() &&
           *first == "three" && *second == "two" && *third == "one";
}

bool expect_console_history_limit() {
    console_calc::ConsoleHistory history;
    for (int index = 1; index <= 11; ++index) {
        history.record("cmd" + std::to_string(index));
    }

    const auto newest = history.previous();
    for (int index = 0; index < 8; ++index) {
        (void)history.previous();
    }
    const auto oldest = history.previous();

    return newest.has_value() && oldest.has_value() && *newest == "cmd11" &&
           *oldest == "cmd2";
}

bool expect_console_history_reset_navigation() {
    console_calc::ConsoleHistory history;
    history.record("one");
    history.record("two");
    (void)history.previous();
    history.reset_navigation();

    const auto latest = history.previous();
    return latest.has_value() && *latest == "two";
}

bool expect_console_history_ignores_empty() {
    console_calc::ConsoleHistory history;
    history.record("");
    history.record("one");

    const auto latest = history.previous();
    const auto oldest = history.previous();
    return latest.has_value() && oldest.has_value() && *latest == "one" && *oldest == "one";
}

}  // namespace

int main() {
    if (!expect_console_history_previous()) {
        return EXIT_FAILURE;
    }

    if (!expect_console_history_limit()) {
        return EXIT_FAILURE;
    }

    if (!expect_console_history_reset_navigation()) {
        return EXIT_FAILURE;
    }

    if (!expect_console_history_ignores_empty()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
