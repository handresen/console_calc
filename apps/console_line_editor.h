#pragma once

#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>

#include "console_history.h"

namespace console_calc {

class ConsoleLineEditor {
public:
    ConsoleLineEditor(std::istream& input, std::ostream& output, ConsoleHistory& history);

    [[nodiscard]] std::optional<std::string> read_line(std::string_view prompt);

private:
    void redraw_input_line(std::string_view prompt, std::string_view buffer,
                           std::size_t cursor) const;
    [[nodiscard]] bool use_interactive_input() const;

    std::istream& input_;
    std::ostream& output_;
    ConsoleHistory& history_;
};

}  // namespace console_calc
