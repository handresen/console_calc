#include "console_calc_app.h"

#include <exception>
#include <istream>
#include <ostream>
#include <optional>
#include <string>

#include "compile_time_constants.h"
#include "console_mode.h"
#include "expression_environment.h"
#include "console_calc/expression_parser.h"

namespace console_calc {

namespace {

[[nodiscard]] std::string join_arguments(std::span<const std::string_view> args) {
    std::string expression;
    for (std::size_t index = 0; index < args.size(); ++index) {
        if (index != 0) {
            expression += ' ';
        }

        expression += args[index];
    }

    return expression;
}

int evaluate_expression(const ExpressionParser& parser, std::string_view expression,
                        const ConstantTable& constants, std::ostream& output,
                        std::ostream& error) {
    try {
        output << parser.evaluate(
            expand_expression_identifiers(expression, constants, DefinitionTable{}, std::nullopt))
               << '\n';
        return 0;
    } catch (const std::exception& ex) {
        error << "error: " << ex.what() << '\n';
        return 1;
    }
}

}  // namespace

int run_console_calc(std::span<const std::string_view> args, std::istream& input,
                     std::ostream& output, std::ostream& error) {
    const ExpressionParser parser;
    const ConstantTable constants = builtin_constant_table();

    if (args.empty()) {
        return run_console_mode(parser, constants, input, output, error);
    }

    return evaluate_expression(parser, join_arguments(args), constants, output, error);
}

}  // namespace console_calc
