#include "expression_conformance_checks.h"

#include <variant>

#include "console_calc/expression_ast.h"
#include "expression_conformance_test_support.h"

namespace console_calc::test {

bool expect_expression_ast_shape(ExpressionParser& parser) {
    using console_calc::BinaryExpression;
    using console_calc::BinaryOperator;
    using console_calc::Expression;
    using console_calc::FillCall;
    using console_calc::Function;
    using console_calc::FunctionCall;
    using console_calc::GuardCall;
    using console_calc::ListWhereCall;
    using console_calc::ListLiteral;
    using console_calc::MapCall;
    using console_calc::NumberLiteral;
    using console_calc::PlaceholderExpression;
    using console_calc::ReduceCall;
    using console_calc::TimedLoopCall;
    using console_calc::UnaryExpression;

    {
        const Expression ast = parser.parse("2 + 3 * 4");
        const auto* root = std::get_if<BinaryExpression>(&ast.node);
        if (root == nullptr || root->op != BinaryOperator::add) {
            return false;
        }
        const auto* lhs = std::get_if<NumberLiteral>(&root->left->node);
        const auto* rhs = std::get_if<BinaryExpression>(&root->right->node);
        const auto* rhs_left = rhs != nullptr ? std::get_if<NumberLiteral>(&rhs->left->node) : nullptr;
        const auto* rhs_right = rhs != nullptr ? std::get_if<NumberLiteral>(&rhs->right->node) : nullptr;
        if (lhs == nullptr || rhs == nullptr || rhs->op != BinaryOperator::multiply ||
            rhs_left == nullptr || rhs_right == nullptr ||
            !almost_equal(lhs->value, 2.0) || !almost_equal(rhs_left->value, 3.0) ||
            !almost_equal(rhs_right->value, 4.0)) {
            return false;
        }
    }

    {
        const Expression ast = parser.parse("(2 + 3) * 4");
        const auto* root = std::get_if<BinaryExpression>(&ast.node);
        const auto* lhs = root != nullptr ? std::get_if<BinaryExpression>(&root->left->node) : nullptr;
        const auto* rhs = root != nullptr ? std::get_if<NumberLiteral>(&root->right->node) : nullptr;
        const auto* lhs_left = lhs != nullptr ? std::get_if<NumberLiteral>(&lhs->left->node) : nullptr;
        const auto* lhs_right = lhs != nullptr ? std::get_if<NumberLiteral>(&lhs->right->node) : nullptr;
        if (root == nullptr || root->op != BinaryOperator::multiply || lhs == nullptr ||
            lhs->op != BinaryOperator::add || rhs == nullptr || lhs_left == nullptr ||
            lhs_right == nullptr || !almost_equal(rhs->value, 4.0) ||
            !almost_equal(lhs_left->value, 2.0) || !almost_equal(lhs_right->value, 3.0)) {
            return false;
        }
    }

    {
        const Expression ast = parser.parse("-2^2");
        const auto* root = std::get_if<UnaryExpression>(&ast.node);
        const auto* operand =
            root != nullptr ? std::get_if<BinaryExpression>(&root->operand->node) : nullptr;
        const auto* lhs = operand != nullptr ? std::get_if<NumberLiteral>(&operand->left->node) : nullptr;
        const auto* rhs = operand != nullptr ? std::get_if<NumberLiteral>(&operand->right->node) : nullptr;
        if (root == nullptr || operand == nullptr || operand->op != BinaryOperator::power ||
            lhs == nullptr || rhs == nullptr || !almost_equal(lhs->value, 2.0) ||
            !almost_equal(rhs->value, 2.0)) {
            return false;
        }
    }

    {
        const Expression ast = parser.parse("2 ^ 3 ^ 2");
        const auto* root = std::get_if<BinaryExpression>(&ast.node);
        const auto* lhs = root != nullptr ? std::get_if<NumberLiteral>(&root->left->node) : nullptr;
        const auto* rhs = root != nullptr ? std::get_if<BinaryExpression>(&root->right->node) : nullptr;
        const auto* rhs_left = rhs != nullptr ? std::get_if<NumberLiteral>(&rhs->left->node) : nullptr;
        const auto* rhs_right = rhs != nullptr ? std::get_if<NumberLiteral>(&rhs->right->node) : nullptr;
        if (root == nullptr || root->op != BinaryOperator::power || lhs == nullptr || rhs == nullptr ||
            rhs->op != BinaryOperator::power || rhs_left == nullptr || rhs_right == nullptr ||
            !almost_equal(lhs->value, 2.0) || !almost_equal(rhs_left->value, 3.0) ||
            !almost_equal(rhs_right->value, 2.0)) {
            return false;
        }
    }

    {
        const Expression ast = parser.parse("2 + 3 & 1");
        const auto* root = std::get_if<BinaryExpression>(&ast.node);
        const auto* lhs = root != nullptr ? std::get_if<BinaryExpression>(&root->left->node) : nullptr;
        const auto* rhs = root != nullptr ? std::get_if<NumberLiteral>(&root->right->node) : nullptr;
        if (root == nullptr || root->op != BinaryOperator::bitwise_and || lhs == nullptr ||
            lhs->op != BinaryOperator::add || rhs == nullptr || !almost_equal(rhs->value, 1.0)) {
            return false;
        }
    }

    {
        const Expression ast = parser.parse("1 + 2 < 4 & 7");
        const auto* root = std::get_if<BinaryExpression>(&ast.node);
        const auto* lhs = root != nullptr ? std::get_if<BinaryExpression>(&root->left->node) : nullptr;
        const auto* rhs = root != nullptr ? std::get_if<NumberLiteral>(&root->right->node) : nullptr;
        const auto* comparison_lhs =
            lhs != nullptr ? std::get_if<BinaryExpression>(&lhs->left->node) : nullptr;
        const auto* comparison_rhs =
            lhs != nullptr ? std::get_if<NumberLiteral>(&lhs->right->node) : nullptr;
        if (root == nullptr || root->op != BinaryOperator::bitwise_and || lhs == nullptr ||
            lhs->op != BinaryOperator::less || comparison_lhs == nullptr ||
            comparison_lhs->op != BinaryOperator::add || comparison_rhs == nullptr ||
            rhs == nullptr || !almost_equal(comparison_rhs->value, 4.0) ||
            !almost_equal(rhs->value, 7.0)) {
            return false;
        }
    }

    {
        const Expression ast = parser.parse("1 + 2 = 3 & 7");
        const auto* root = std::get_if<BinaryExpression>(&ast.node);
        const auto* lhs = root != nullptr ? std::get_if<BinaryExpression>(&root->left->node) : nullptr;
        const auto* rhs = root != nullptr ? std::get_if<NumberLiteral>(&root->right->node) : nullptr;
        const auto* comparison_lhs =
            lhs != nullptr ? std::get_if<BinaryExpression>(&lhs->left->node) : nullptr;
        const auto* comparison_rhs =
            lhs != nullptr ? std::get_if<NumberLiteral>(&lhs->right->node) : nullptr;
        if (root == nullptr || root->op != BinaryOperator::bitwise_and || lhs == nullptr ||
            lhs->op != BinaryOperator::equal || comparison_lhs == nullptr ||
            comparison_lhs->op != BinaryOperator::add || comparison_rhs == nullptr ||
            rhs == nullptr || !almost_equal(comparison_rhs->value, 3.0) ||
            !almost_equal(rhs->value, 7.0)) {
            return false;
        }
    }

    {
        const Expression ast = parser.parse("2 | 4 & 1");
        const auto* root = std::get_if<BinaryExpression>(&ast.node);
        const auto* lhs = root != nullptr ? std::get_if<NumberLiteral>(&root->left->node) : nullptr;
        const auto* rhs = root != nullptr ? std::get_if<BinaryExpression>(&root->right->node) : nullptr;
        if (root == nullptr || root->op != BinaryOperator::bitwise_or || lhs == nullptr ||
            rhs == nullptr || rhs->op != BinaryOperator::bitwise_and ||
            !almost_equal(lhs->value, 2.0)) {
            return false;
        }
    }

    {
        const Expression ast = parser.parse("2 <= 3 > 0");
        const auto* root = std::get_if<BinaryExpression>(&ast.node);
        const auto* lhs = root != nullptr ? std::get_if<BinaryExpression>(&root->left->node) : nullptr;
        const auto* rhs = root != nullptr ? std::get_if<NumberLiteral>(&root->right->node) : nullptr;
        if (root == nullptr || root->op != BinaryOperator::greater || lhs == nullptr ||
            lhs->op != BinaryOperator::less_equal || rhs == nullptr ||
            !almost_equal(rhs->value, 0.0)) {
            return false;
        }
    }

    {
        const Expression ast = parser.parse("abs(2) + sqrt(9)");
        const auto* root = std::get_if<BinaryExpression>(&ast.node);
        const auto* lhs = root != nullptr ? std::get_if<FunctionCall>(&root->left->node) : nullptr;
        const auto* rhs = root != nullptr ? std::get_if<FunctionCall>(&root->right->node) : nullptr;
        if (root == nullptr || root->op != BinaryOperator::add || lhs == nullptr || rhs == nullptr ||
            lhs->function != Function::abs || rhs->function != Function::sqrt ||
            lhs->arguments.size() != 1 || rhs->arguments.size() != 1) {
            return false;
        }
    }

    {
        const Expression ast = parser.parse("sum({2, 3, 4})");
        const auto* root = std::get_if<FunctionCall>(&ast.node);
        const auto* list = root != nullptr ? std::get_if<ListLiteral>(&root->arguments[0]->node) : nullptr;
        if (root == nullptr || root->function != Function::sum || root->arguments.size() != 1 ||
            list == nullptr || list->elements.size() != 3) {
            return false;
        }
    }

    {
        const Expression ast = parser.parse("map_at({2, 3, 4, 5}, _ + 1, 1, 2, 4)");
        const auto* root = std::get_if<MapCall>(&ast.node);
        const auto* mapped =
            root != nullptr ? std::get_if<BinaryExpression>(&root->mapped_expression->node) : nullptr;
        const auto* start =
            root != nullptr ? std::get_if<NumberLiteral>(&root->start_argument->node) : nullptr;
        const auto* step =
            root != nullptr ? std::get_if<NumberLiteral>(&root->step_argument->node) : nullptr;
        const auto* count =
            root != nullptr ? std::get_if<NumberLiteral>(&root->count_argument->node) : nullptr;
        if (root == nullptr || mapped == nullptr || root->mapped_expression == nullptr ||
            root->start_argument == nullptr || root->step_argument == nullptr ||
            root->count_argument == nullptr || !root->preserve_unmapped ||
            !std::holds_alternative<PlaceholderExpression>(mapped->left->node) ||
            start == nullptr || step == nullptr || count == nullptr ||
            !almost_equal(start->value, 1.0) || !almost_equal(step->value, 2.0) ||
            !almost_equal(count->value, 4.0)) {
            return false;
        }
    }

    {
        const Expression ast = parser.parse("list_where({2, 3, 4, 5}, _ <= 3)");
        const auto* root = std::get_if<ListWhereCall>(&ast.node);
        const auto* list =
            root != nullptr ? std::get_if<ListLiteral>(&root->list_argument->node) : nullptr;
        const auto* predicate =
            root != nullptr ? std::get_if<BinaryExpression>(&root->predicate_expression->node)
                            : nullptr;
        const auto* rhs =
            predicate != nullptr ? std::get_if<NumberLiteral>(&predicate->right->node) : nullptr;
        if (root == nullptr || list == nullptr || list->elements.size() != 4 ||
            predicate == nullptr || predicate->op != BinaryOperator::less_equal ||
            !std::holds_alternative<PlaceholderExpression>(predicate->left->node) ||
            rhs == nullptr || !almost_equal(rhs->value, 3.0)) {
            return false;
        }
    }

    {
        const Expression ast = parser.parse("guard(1 / 0, 5)");
        const auto* root = std::get_if<GuardCall>(&ast.node);
        const auto* guarded =
            root != nullptr ? std::get_if<BinaryExpression>(&root->guarded_expression->node) : nullptr;
        const auto* fallback =
            root != nullptr ? std::get_if<NumberLiteral>(&root->fallback_expression->node) : nullptr;
        if (root == nullptr || guarded == nullptr || guarded->op != BinaryOperator::divide ||
            fallback == nullptr || !almost_equal(fallback->value, 5.0)) {
            return false;
        }
    }

    {
        const Expression ast = parser.parse("reduce({2, 3}, *)");
        const auto* root = std::get_if<ReduceCall>(&ast.node);
        const auto* list =
            root != nullptr ? std::get_if<ListLiteral>(&root->list_argument->node) : nullptr;
        if (root == nullptr || root->reduction_operator != BinaryOperator::multiply ||
            list == nullptr || list->elements.size() != 2) {
            return false;
        }
    }

    {
        const Expression ast = parser.parse("timed_loop(1 + 2, 3)");
        const auto* root = std::get_if<TimedLoopCall>(&ast.node);
        const auto* iteration_count =
            root != nullptr ? std::get_if<NumberLiteral>(&root->iteration_count->node) : nullptr;
        if (root == nullptr || root->loop_expression == nullptr || iteration_count == nullptr ||
            !almost_equal(iteration_count->value, 3.0)) {
            return false;
        }
    }

    {
        const Expression ast = parser.parse("fill(1 + 2, 3)");
        const auto* root = std::get_if<FillCall>(&ast.node);
        const auto* iteration_count =
            root != nullptr ? std::get_if<NumberLiteral>(&root->iteration_count->node) : nullptr;
        if (root == nullptr || root->fill_expression == nullptr || iteration_count == nullptr ||
            !almost_equal(iteration_count->value, 3.0)) {
            return false;
        }
    }

    {
        const Expression range_ast = parser.parse("range(2, 4, 3)");
        const auto* range_call = std::get_if<FunctionCall>(&range_ast.node);
        const Expression geom_ast = parser.parse("geom(2, 4, 3)");
        const auto* geom_call = std::get_if<FunctionCall>(&geom_ast.node);
        const Expression repeat_ast = parser.parse("repeat(2, 4)");
        const auto* repeat_call = std::get_if<FunctionCall>(&repeat_ast.node);
        const Expression linspace_ast = parser.parse("linspace(1, 4, 4)");
        const auto* linspace_call = std::get_if<FunctionCall>(&linspace_ast.node);
        const Expression powers_ast = parser.parse("powers(-1, 4)");
        const auto* powers_call = std::get_if<FunctionCall>(&powers_ast.node);
        if (range_call == nullptr || range_call->function != Function::range ||
            range_call->arguments.size() != 3 || geom_call == nullptr ||
            geom_call->function != Function::geom || geom_call->arguments.size() != 3 ||
            repeat_call == nullptr || repeat_call->function != Function::repeat ||
            repeat_call->arguments.size() != 2 || linspace_call == nullptr ||
            linspace_call->function != Function::linspace || linspace_call->arguments.size() != 3 ||
            powers_call == nullptr || powers_call->function != Function::powers ||
            powers_call->arguments.size() != 2) {
            return false;
        }
    }

    return true;
}

}  // namespace console_calc::test
