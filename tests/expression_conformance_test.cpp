#include <cmath>
#include <cstdlib>
#include <exception>
#include <variant>

#include "console_calc/expression_ast.h"
#include "console_calc/expression_parser.h"
#include "expression_case_loader.h"

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

bool expect_value_api(console_calc::ExpressionParser& parser) {
    const console_calc::Value scalar = parser.evaluate_value("1 + 2");
    const auto* scalar_value = std::get_if<double>(&scalar);
    if (scalar_value == nullptr || !almost_equal(*scalar_value, 3.0)) {
        return false;
    }

    const console_calc::Value list = parser.evaluate_value("{1, 2, 3}");
    const auto* list_value = std::get_if<console_calc::ListValue>(&list);
    if (list_value == nullptr || list_value->size() != 3) {
        return false;
    }

    return almost_equal((*list_value)[0], 1.0) && almost_equal((*list_value)[1], 2.0) &&
           almost_equal((*list_value)[2], 3.0);
}

bool expect_value_api_boundaries(console_calc::ExpressionParser& parser) {
    try {
        (void)parser.evaluate("{1, 2, 3}");
        return false;
    } catch (const std::invalid_argument&) {
    } catch (const std::exception&) {
        return false;
    }

    try {
        (void)parser.evaluate("sum(1)");
        return false;
    } catch (const std::invalid_argument&) {
    } catch (const std::exception&) {
        return false;
    }

    try {
        (void)parser.evaluate_value("sum({})");
    } catch (const std::exception&) {
        return false;
    }

    return true;
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

bool expect_parenthesized_ast_shape(console_calc::ExpressionParser& parser) {
    using console_calc::BinaryExpression;
    using console_calc::BinaryOperator;
    using console_calc::Expression;
    using console_calc::NumberLiteral;

    const Expression ast = parser.parse("(2 + 3) * 4");
    const auto* root = std::get_if<BinaryExpression>(&ast.node);
    if (root == nullptr || root->op != BinaryOperator::multiply) {
        return false;
    }

    const auto* rhs = std::get_if<NumberLiteral>(&root->right->node);
    if (rhs == nullptr || !almost_equal(rhs->value, 4.0)) {
        return false;
    }

    const auto* lhs = std::get_if<BinaryExpression>(&root->left->node);
    if (lhs == nullptr || lhs->op != BinaryOperator::add) {
        return false;
    }

    const auto* lhs_left = std::get_if<NumberLiteral>(&lhs->left->node);
    const auto* lhs_right = std::get_if<NumberLiteral>(&lhs->right->node);
    return lhs_left != nullptr && lhs_right != nullptr &&
           almost_equal(lhs_left->value, 2.0) && almost_equal(lhs_right->value, 3.0);
}

bool expect_unary_minus_ast_shape(console_calc::ExpressionParser& parser) {
    using console_calc::BinaryExpression;
    using console_calc::BinaryOperator;
    using console_calc::Expression;
    using console_calc::NumberLiteral;
    using console_calc::UnaryExpression;

    const Expression ast = parser.parse("-2^2");
    const auto* root = std::get_if<UnaryExpression>(&ast.node);
    if (root == nullptr) {
        return false;
    }

    const auto* operand = std::get_if<BinaryExpression>(&root->operand->node);
    if (operand == nullptr || operand->op != BinaryOperator::power) {
        return false;
    }

    const auto* lhs = std::get_if<NumberLiteral>(&operand->left->node);
    const auto* rhs = std::get_if<NumberLiteral>(&operand->right->node);
    return lhs != nullptr && rhs != nullptr && almost_equal(lhs->value, 2.0) &&
           almost_equal(rhs->value, 2.0);
}

bool expect_power_ast_shape(console_calc::ExpressionParser& parser) {
    using console_calc::BinaryExpression;
    using console_calc::BinaryOperator;
    using console_calc::Expression;
    using console_calc::NumberLiteral;

    const Expression ast = parser.parse("2 ^ 3 ^ 2");
    const auto* root = std::get_if<BinaryExpression>(&ast.node);
    if (root == nullptr || root->op != BinaryOperator::power) {
        return false;
    }

    const auto* lhs = std::get_if<NumberLiteral>(&root->left->node);
    if (lhs == nullptr || !almost_equal(lhs->value, 2.0)) {
        return false;
    }

    const auto* rhs = std::get_if<BinaryExpression>(&root->right->node);
    if (rhs == nullptr || rhs->op != BinaryOperator::power) {
        return false;
    }

    const auto* rhs_left = std::get_if<NumberLiteral>(&rhs->left->node);
    const auto* rhs_right = std::get_if<NumberLiteral>(&rhs->right->node);
    return rhs_left != nullptr && rhs_right != nullptr &&
           almost_equal(rhs_left->value, 3.0) && almost_equal(rhs_right->value, 2.0);
}

bool expect_bitwise_and_ast_shape(console_calc::ExpressionParser& parser) {
    using console_calc::BinaryExpression;
    using console_calc::BinaryOperator;
    using console_calc::Expression;
    using console_calc::NumberLiteral;

    const Expression ast = parser.parse("2 + 3 & 1");
    const auto* root = std::get_if<BinaryExpression>(&ast.node);
    if (root == nullptr || root->op != BinaryOperator::bitwise_and) {
        return false;
    }

    const auto* rhs = std::get_if<NumberLiteral>(&root->right->node);
    if (rhs == nullptr || !almost_equal(rhs->value, 1.0)) {
        return false;
    }

    const auto* lhs = std::get_if<BinaryExpression>(&root->left->node);
    if (lhs == nullptr || lhs->op != BinaryOperator::add) {
        return false;
    }

    const auto* lhs_left = std::get_if<NumberLiteral>(&lhs->left->node);
    const auto* lhs_right = std::get_if<NumberLiteral>(&lhs->right->node);
    return lhs_left != nullptr && lhs_right != nullptr &&
           almost_equal(lhs_left->value, 2.0) && almost_equal(lhs_right->value, 3.0);
}

bool expect_bitwise_or_ast_shape(console_calc::ExpressionParser& parser) {
    using console_calc::BinaryExpression;
    using console_calc::BinaryOperator;
    using console_calc::Expression;
    using console_calc::NumberLiteral;

    const Expression ast = parser.parse("2 | 4 & 1");
    const auto* root = std::get_if<BinaryExpression>(&ast.node);
    if (root == nullptr || root->op != BinaryOperator::bitwise_or) {
        return false;
    }

    const auto* lhs = std::get_if<NumberLiteral>(&root->left->node);
    if (lhs == nullptr || !almost_equal(lhs->value, 2.0)) {
        return false;
    }

    const auto* rhs = std::get_if<BinaryExpression>(&root->right->node);
    if (rhs == nullptr || rhs->op != BinaryOperator::bitwise_and) {
        return false;
    }

    const auto* rhs_left = std::get_if<NumberLiteral>(&rhs->left->node);
    const auto* rhs_right = std::get_if<NumberLiteral>(&rhs->right->node);
    return rhs_left != nullptr && rhs_right != nullptr &&
           almost_equal(rhs_left->value, 4.0) && almost_equal(rhs_right->value, 1.0);
}

bool expect_function_ast_shape(console_calc::ExpressionParser& parser) {
    using console_calc::BinaryExpression;
    using console_calc::BinaryOperator;
    using console_calc::Expression;
    using console_calc::Function;
    using console_calc::FunctionCall;
    using console_calc::NumberLiteral;

    const Expression ast = parser.parse("sin(2) + pow(3, 4)");
    const auto* root = std::get_if<BinaryExpression>(&ast.node);
    if (root == nullptr || root->op != BinaryOperator::add) {
        return false;
    }

    const auto* lhs = std::get_if<FunctionCall>(&root->left->node);
    if (lhs == nullptr || lhs->function != Function::sin || lhs->arguments.size() != 1) {
        return false;
    }

    const auto* lhs_arg = std::get_if<NumberLiteral>(&lhs->arguments[0]->node);
    if (lhs_arg == nullptr || !almost_equal(lhs_arg->value, 2.0)) {
        return false;
    }

    const auto* rhs = std::get_if<FunctionCall>(&root->right->node);
    if (rhs == nullptr || rhs->function != Function::pow || rhs->arguments.size() != 2) {
        return false;
    }

    const auto* rhs_left = std::get_if<NumberLiteral>(&rhs->arguments[0]->node);
    const auto* rhs_right = std::get_if<NumberLiteral>(&rhs->arguments[1]->node);
    return rhs_left != nullptr && rhs_right != nullptr &&
           almost_equal(rhs_left->value, 3.0) && almost_equal(rhs_right->value, 4.0);
}

bool expect_sum_ast_shape(console_calc::ExpressionParser& parser) {
    using console_calc::Expression;
    using console_calc::Function;
    using console_calc::FunctionCall;
    using console_calc::ListLiteral;
    using console_calc::NumberLiteral;

    const Expression ast = parser.parse("sum({2, 3, 4})");
    const auto* root = std::get_if<FunctionCall>(&ast.node);
    if (root == nullptr || root->function != Function::sum || root->arguments.size() != 1) {
        return false;
    }

    const auto* list = std::get_if<ListLiteral>(&root->arguments[0]->node);
    if (list == nullptr || list->elements.size() != 3) {
        return false;
    }

    for (std::size_t index = 0; index < list->elements.size(); ++index) {
        const auto* element = std::get_if<NumberLiteral>(&list->elements[index]->node);
        if (element == nullptr || !almost_equal(element->value, static_cast<double>(index + 2))) {
            return false;
        }
    }

    return true;
}

}  // namespace

int main() {
    console_calc::ExpressionParser parser;

    if (!expect_ast_shape(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_value_api(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_value_api_boundaries(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_parenthesized_ast_shape(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_unary_minus_ast_shape(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_power_ast_shape(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_bitwise_and_ast_shape(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_bitwise_or_ast_shape(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_function_ast_shape(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_sum_ast_shape(parser)) {
        return EXIT_FAILURE;
    }

    const auto expression_cases =
        console_calc::test::load_expression_cases(CONSOLE_CALC_TEST_DATA_DIR);
    for (const auto& expression_case : expression_cases) {
        if (expression_case.expect_invalid) {
            if (!expect_invalid(parser, expression_case.expression)) {
                return EXIT_FAILURE;
            }
            continue;
        }

        if (!expect_value(parser, expression_case.expression, expression_case.expected_value)) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
