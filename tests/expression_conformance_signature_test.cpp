#include "expression_conformance_checks.h"

#include <exception>
#include <string>

namespace console_calc::test {

bool expect_expression_function_signature_errors(ExpressionParser& parser) {
    struct Case {
        const char* expression;
        const char* expected;
    };

    const Case cases[] = {
        {"pow(2)", "function 'pow' expects pow(x, y)"},
        {"range(1)", "function 'range' expects range(start, count[, step])"},
        {"guard(1 / 0)", "function 'guard' expects guard(expr, fallback)"},
        {"timed_loop(1)", "function 'timed_loop' expects timed_loop(expr, count)"},
        {"fill(1)", "function 'fill' expects fill(expr, count)"},
        {"map_at({1, 2}, _ + 1, 0, 2, 4, 5)",
         "function 'map_at' expects map_at(list, expr[, start[, step[, count]]])"},
        {"map({1, 2}, _ + 1, 0, 2, 4, 5)",
         "function 'map' expects map(list, expr[, start[, step[, count]]])"},
        {"list_where({1, 2}, _ + 1, 0)",
         "function 'list_where' expects list_where(list, expr)"},
        {"to_poslist()", "function 'to_poslist' expects to_poslist(list)"},
        {"to_list()", "function 'to_list' expects to_list(poslist)"},
        {"rand(1, 2, 3)", "function 'rand' expects rand([min, max])"},
    };

    for (const auto& test_case : cases) {
        try {
            (void)parser.parse(test_case.expression);
            return false;
        } catch (const std::invalid_argument& error) {
            if (std::string(error.what()) != test_case.expected) {
                return false;
            }
        } catch (const std::exception&) {
            return false;
        }
    }

    return true;
}

}  // namespace console_calc::test
