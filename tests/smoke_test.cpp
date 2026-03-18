#include <cmath>
#include <cstdlib>
#include <exception>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "console_calc/expression_ast.h"
#include "console_calc/expression_parser.h"

namespace {

bool almost_equal(double lhs, double rhs) {
    return std::fabs(lhs - rhs) < 1e-12;
}

bool expect_value(console_calc::ExpressionParser& parser, const std::string& expression,
                  double expected) {
    try {
        return almost_equal(parser.evaluate(expression), expected);
    } catch (const std::exception&) {
        return false;
    }
}

bool expect_invalid(console_calc::ExpressionParser& parser, const std::string& expression) {
    try {
        (void)parser.evaluate(expression);
        return false;
    } catch (const std::invalid_argument&) {
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool expect_ast_shape(console_calc::ExpressionParser& parser) {
    using console_calc::BinaryExpression;
    using console_calc::BinaryOperator;
    using console_calc::Expression;
    using console_calc::NumberLiteral;

    const Expression ast = parser.parse("2 + 3 * 4");
    const auto* root = std::get_if<BinaryExpression>(&ast.node);
    if (root == nullptr || root->op != BinaryOperator::add) {
        return false;
    }

    const auto* lhs = std::get_if<NumberLiteral>(&root->left->node);
    if (lhs == nullptr || !almost_equal(lhs->value, 2.0)) {
        return false;
    }

    const auto* rhs = std::get_if<BinaryExpression>(&root->right->node);
    if (rhs == nullptr || rhs->op != BinaryOperator::multiply) {
        return false;
    }

    const auto* rhs_left = std::get_if<NumberLiteral>(&rhs->left->node);
    const auto* rhs_right = std::get_if<NumberLiteral>(&rhs->right->node);
    return rhs_left != nullptr && rhs_right != nullptr &&
           almost_equal(rhs_left->value, 3.0) && almost_equal(rhs_right->value, 4.0);
}

}  // namespace

int main() {
    console_calc::ExpressionParser parser;

    if (!expect_ast_shape(parser)) {
        return EXIT_FAILURE;
    }

    const std::vector<std::pair<std::string, double>> valid_cases = {
        {"1+1", 2.0},
        {"2 + 3 * 4", 14.0},
        {"20 / 5 - 1", 3.0},
        {"1.5 + 2.25", 3.75},
        {".5 * 8", 4.0},
        {"1.3e2 / 2", 65.0},
        {"10. - 3.5 + .5", 7.0},
        {"8 / 4 * 2", 4.0},
        {"18 / 3 / 2", 3.0},
        {"10 - 6 / 2", 7.0},
        {"2 * 3 + 4 * 5", 26.0},
        {"100 / 10 + 6 * 3 - 4", 24.0},
    };

    for (const auto& [expression, expected] : valid_cases) {
        if (!expect_value(parser, expression, expected)) {
            return EXIT_FAILURE;
        }
    }

    const std::vector<std::string> invalid_cases = {
        "",
        "+1",
        "1+",
        "1++2",
        "-3",
        ".",
        "1e",
        "1e+",
        "(1+2)",
        "2*-3",
        "2a+1",
    };

    for (const auto& expression : invalid_cases) {
        if (!expect_invalid(parser, expression)) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
