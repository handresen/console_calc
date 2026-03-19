#include "console_session.h"

#include <istream>
#include <ostream>
#include <string>

#include "console_calc/expression_parser.h"
#include "console_calc/scalar_value.h"
#include "console_listing.h"

namespace console_calc {

namespace {

constexpr std::string_view k_prompt_color = "\x1b[32m";
constexpr std::string_view k_color_reset = "\x1b[0m";

void print_result(std::ostream& output, const Value& value, IntegerDisplayMode mode) {
    if (const auto* integer = std::get_if<std::int64_t>(&value)) {
        output << format_scalar(ScalarValue{*integer}, mode) << '\n';
        return;
    }

    if (const auto* scalar = std::get_if<double>(&value)) {
        output << *scalar << '\n';
        return;
    }

    output << format_console_value(value, mode) << '\n';
}

}  // namespace

ConsoleSession::ConsoleSession(const ExpressionParser& parser, const ConstantTable& constants,
                               std::istream& input, std::ostream& output,
                               std::ostream& error)
    : input_(input),
      output_(output),
      error_(error),
      engine_(parser, constants),
      line_editor_(input_, output_, history_) {}

ConsoleSession::ConsoleSession(const ExpressionParser& parser, const ConstantTable& constants,
                               std::istream& input, std::ostream& output,
                               std::ostream& error,
                               CurrencyRateProvider* currency_rate_provider,
                               std::chrono::milliseconds currency_rate_timeout,
                               bool auto_refresh_currency_rates)
    : input_(input),
      output_(output),
      error_(error),
      engine_(parser, constants, currency_rate_provider, currency_rate_timeout,
              auto_refresh_currency_rates),
      line_editor_(input_, output_, history_) {}

int ConsoleSession::run() {
    engine_.initialize();

    while (true) {
        const auto line = line_editor_.read_line(prompt_text());
        if (!line.has_value()) {
            return 0;
        }

        const int result = handle_line(*line);
        if (result >= 0) {
            return result;
        }
    }
}

int ConsoleSession::handle_line(std::string_view line) {
    const ConsoleEngineCommandResult result = engine_.submit(line);
    for (const auto& value : result.emitted_values) {
        print_result(output_, value, engine_.display_mode());
    }
    for (const auto& output_line : result.output_lines) {
        if (output_line.empty()) {
            continue;
        }
        output_ << output_line;
        if (!output_line.ends_with('\n')) {
            output_ << '\n';
        }
    }
    for (const auto& error_line : result.error_lines) {
        error_ << "error: " << error_line << '\n';
    }

    return result.should_exit ? 0 : -1;
}

std::string ConsoleSession::prompt_text() const {
    return std::string(k_prompt_color) + std::to_string(engine_.stack_depth()) + '>' +
           std::string(k_color_reset);
}

}  // namespace console_calc
