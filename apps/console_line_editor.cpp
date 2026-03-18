#include "console_line_editor.h"

#include <cctype>
#include <iostream>
#include <istream>
#include <ostream>

#if defined(__unix__) || defined(__APPLE__)
#include <termios.h>
#include <unistd.h>
#endif

namespace console_calc {

namespace {

constexpr std::string_view k_clear_line = "\r\x1b[2K";

enum class EscapeAction {
    none,
    history_previous,
    cursor_left,
    cursor_right,
    cursor_home,
    cursor_end,
};

[[nodiscard]] EscapeAction read_escape_action(std::istream& input) {
    if (input.peek() != '[') {
        return EscapeAction::none;
    }

    (void)input.get();
    const int code = input.get();
    switch (code) {
    case 'A':
        return EscapeAction::history_previous;
    case 'C':
        return EscapeAction::cursor_right;
    case 'D':
        return EscapeAction::cursor_left;
    case 'F':
    case 'K':
        return EscapeAction::cursor_end;
    case 'H':
        return EscapeAction::cursor_home;
    case '1':
    case '4':
    case '7':
    case '8': {
        if (input.get() != '~') {
            return EscapeAction::none;
        }
        if (code == '1' || code == '7') {
            return EscapeAction::cursor_home;
        }
        return EscapeAction::cursor_end;
    }
    default:
        return EscapeAction::none;
    }
}

#if defined(__unix__) || defined(__APPLE__)
class TerminalRawModeGuard {
public:
    explicit TerminalRawModeGuard(bool enabled) : enabled_(enabled) {
        if (!enabled_) {
            return;
        }

        if (::tcgetattr(STDIN_FILENO, &original_) != 0) {
            enabled_ = false;
            return;
        }

        termios raw = original_;
        raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        if (::tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) {
            enabled_ = false;
        }
    }

    ~TerminalRawModeGuard() {
        if (enabled_) {
            (void)::tcsetattr(STDIN_FILENO, TCSANOW, &original_);
        }
    }

    TerminalRawModeGuard(const TerminalRawModeGuard&) = delete;
    TerminalRawModeGuard& operator=(const TerminalRawModeGuard&) = delete;

private:
    bool enabled_;
    termios original_{};
};
#endif

}  // namespace

ConsoleLineEditor::ConsoleLineEditor(std::istream& input, std::ostream& output,
                                     ConsoleHistory& history)
    : input_(input), output_(output), history_(history) {}

std::optional<std::string> ConsoleLineEditor::read_line(std::string_view prompt) {
    output_ << prompt;
    output_.flush();

    if (!use_interactive_input()) {
        std::string line;
        if (!std::getline(input_, line)) {
            return std::nullopt;
        }
        return line;
    }

#if defined(__unix__) || defined(__APPLE__)
    TerminalRawModeGuard raw_mode(true);
#endif
    history_.reset_navigation();
    std::string buffer;
    std::size_t cursor = 0;

    while (true) {
        const int next = input_.get();
        if (next == EOF) {
            return std::nullopt;
        }

        const char ch = static_cast<char>(next);
        if (ch == '\n' || ch == '\r') {
            output_ << '\n';
            history_.record(buffer);
            return buffer;
        }

        if (ch == '\x7f' || ch == '\b') {
            if (cursor > 0) {
                buffer.erase(cursor - 1, 1);
                --cursor;
                redraw_input_line(prompt, buffer, cursor);
            }
            continue;
        }

        if (ch == '\x1b') {
            switch (read_escape_action(input_)) {
            case EscapeAction::history_previous:
                if (const auto previous = history_.previous()) {
                    buffer = *previous;
                    cursor = buffer.size();
                    redraw_input_line(prompt, buffer, cursor);
                }
                break;
            case EscapeAction::cursor_right:
                if (cursor < buffer.size()) {
                    ++cursor;
                    redraw_input_line(prompt, buffer, cursor);
                }
                break;
            case EscapeAction::cursor_left:
                if (cursor > 0) {
                    --cursor;
                    redraw_input_line(prompt, buffer, cursor);
                }
                break;
            case EscapeAction::cursor_home:
                if (cursor != 0) {
                    cursor = 0;
                    redraw_input_line(prompt, buffer, cursor);
                }
                break;
            case EscapeAction::cursor_end:
                if (cursor != buffer.size()) {
                    cursor = buffer.size();
                    redraw_input_line(prompt, buffer, cursor);
                }
                break;
            case EscapeAction::none:
                break;
            }
            continue;
        }

        if (!std::isprint(static_cast<unsigned char>(ch))) {
            continue;
        }

        buffer.insert(buffer.begin() + static_cast<std::ptrdiff_t>(cursor), ch);
        ++cursor;
        redraw_input_line(prompt, buffer, cursor);
    }
}

void ConsoleLineEditor::redraw_input_line(std::string_view prompt, std::string_view buffer,
                                          std::size_t cursor) const {
    output_ << k_clear_line << prompt << buffer;
    if (cursor < buffer.size()) {
        output_ << '\r' << prompt << buffer.substr(0, cursor);
    }
    output_.flush();
}

bool ConsoleLineEditor::use_interactive_input() const {
#if defined(__unix__) || defined(__APPLE__)
    return &input_ == &std::cin && ::isatty(STDIN_FILENO) != 0;
#else
    return false;
#endif
}

}  // namespace console_calc
