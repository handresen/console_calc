#include "console_mode.h"

#include "console_session.h"

namespace console_calc {

int run_console_mode(const ExpressionParser& parser, const ConstantTable& constants,
                     std::istream& input, std::ostream& output, std::ostream& error) {
    ConsoleSession session(parser, constants, input, output, error);
    return session.run();
}

}  // namespace console_calc
