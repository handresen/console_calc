#include "console_mode.h"

#include <cctype>
#include <exception>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include "console_calc/expression_parser.h"

namespace console_calc {

namespace {

[[nodiscard]] std::string trim(std::string_view text) {
    std::size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) {
        ++begin;
    }

    std::size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }

    return std::string(text.substr(begin, end - begin));
}

}  // namespace

int run_console_mode(const ExpressionParser& parser, std::istream& input, std::ostream& output,
                     std::ostream& error) {
    std::string line;
    while (std::getline(input, line)) {
        const std::string trimmed = trim(line);
        if (trimmed.empty()) {
            continue;
        }

        if (trimmed == "q" || trimmed == "Q") {
            return 0;
        }

        try {
            output << parser.evaluate(trimmed) << '\n';
        } catch (const std::exception& ex) {
            error << "error: " << ex.what() << '\n';
        }
    }

    return 0;
}

}  // namespace console_calc
