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
        {"to_list()", "function 'to_list' expects to_list(poslist|multi_pos_list)"},
        {"densify_path({pos(0, 0), pos(0, 1)})",
         "function 'densify_path' expects densify_path(poslist|multi_pos_list, count)"},
        {"offset_path({pos(0, 0), pos(0, 1)}, 1)",
         "function 'offset_path' expects offset_path(poslist|multi_pos_list, offset_x_m, offset_y_m)"},
        {"rotate_path({pos(0, 0), pos(0, 1)}, 0)",
         "function 'rotate_path' expects rotate_path(poslist|multi_pos_list, center_index, degrees)"},
        {"scale_path({pos(0, 0), pos(0, 1)})",
         "function 'scale_path' expects scale_path(poslist|multi_pos_list, scale_factor)"},
        {"simplify_path({pos(0, 0), pos(0, 1)})",
         "function 'simplify_path' expects simplify_path(poslist|multi_pos_list, tolerance_m)"},
        {"compress_path({pos(0, 0), pos(0, 1)})",
         "function 'compress_path' expects compress_path(poslist|multi_pos_list, count[, max_points])"},
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
