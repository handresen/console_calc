#include <cmath>
#include <cstdlib>
#include <exception>
#include <string>
#include <variant>

#include "console_calc/expression_ast.h"
#include "console_calc/expression_parser.h"
#include "console_calc/scalar_value.h"
#include "expression_case_loader.h"

namespace {

bool almost_equal(double lhs, double rhs) {
    return std::fabs(lhs - rhs) < 1e-12;
}

bool almost_equal(double lhs, double rhs, double tolerance) {
    return std::fabs(lhs - rhs) < tolerance;
}

bool almost_equal(const console_calc::ScalarValue& lhs, double rhs) {
    return almost_equal(console_calc::scalar_to_double(lhs), rhs);
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
    if (!std::holds_alternative<std::int64_t>(scalar) ||
        !almost_equal(console_calc::scalar_to_double(std::get<std::int64_t>(scalar)), 3.0)) {
        return false;
    }

    const console_calc::Value list = parser.evaluate_value("{1, 2, 3}");
    const auto* list_value = std::get_if<console_calc::ListValue>(&list);
    if (list_value == nullptr || list_value->size() != 3) {
        return false;
    }

    return almost_equal(console_calc::scalar_to_double((*list_value)[0]), 1.0) &&
           almost_equal(console_calc::scalar_to_double((*list_value)[1]), 2.0) &&
           almost_equal(console_calc::scalar_to_double((*list_value)[2]), 3.0);
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

    const console_calc::Value first_list = parser.evaluate_value("first(2, {1, 2, 3})");
    const auto* first_values = std::get_if<console_calc::ListValue>(&first_list);
    if (first_values == nullptr || first_values->size() != 2 ||
        !almost_equal(console_calc::scalar_to_double((*first_values)[0]), 1.0) ||
        !almost_equal(console_calc::scalar_to_double((*first_values)[1]), 2.0)) {
        return false;
    }

    const console_calc::Value dropped_list = parser.evaluate_value("drop(2, {1, 2, 3, 4})");
    const auto* dropped_values = std::get_if<console_calc::ListValue>(&dropped_list);
    if (dropped_values == nullptr || dropped_values->size() != 2 ||
        !almost_equal(console_calc::scalar_to_double((*dropped_values)[0]), 3.0) ||
        !almost_equal(console_calc::scalar_to_double((*dropped_values)[1]), 4.0)) {
        return false;
    }

    if (!almost_equal(parser.evaluate("first(1, {2}) + 3"), 5.0)) {
        return false;
    }

    if (!almost_equal(parser.evaluate("drop(2, {1, 2, 3}) + 4"), 7.0)) {
        return false;
    }

    if (!almost_equal(parser.evaluate("sin({0})"), 0.0)) {
        return false;
    }

    const console_calc::Value mapped_list =
        parser.evaluate_value("map({0, 1.5707963267948966}, sin(_))");
    const auto* mapped_values = std::get_if<console_calc::ListValue>(&mapped_list);
    if (mapped_values == nullptr || mapped_values->size() != 2 ||
        !almost_equal(console_calc::scalar_to_double((*mapped_values)[0]), 0.0) ||
        !almost_equal(console_calc::scalar_to_double((*mapped_values)[1]), 1.0)) {
        return false;
    }

    const console_calc::Value generated_range = parser.evaluate_value("range(2, 4, 3)");
    const auto* generated_values = std::get_if<console_calc::ListValue>(&generated_range);
    if (generated_values == nullptr || generated_values->size() != 4 ||
        !almost_equal(console_calc::scalar_to_double((*generated_values)[0]), 2.0) ||
        !almost_equal(console_calc::scalar_to_double((*generated_values)[1]), 5.0) ||
        !almost_equal(console_calc::scalar_to_double((*generated_values)[2]), 8.0) ||
        !almost_equal(console_calc::scalar_to_double((*generated_values)[3]), 11.0)) {
        return false;
    }

    const console_calc::Value generated_geom = parser.evaluate_value("geom(3, 4, 3)");
    const auto* geometric_values = std::get_if<console_calc::ListValue>(&generated_geom);
    if (geometric_values == nullptr || geometric_values->size() != 4 ||
        !almost_equal(console_calc::scalar_to_double((*geometric_values)[0]), 3.0) ||
        !almost_equal(console_calc::scalar_to_double((*geometric_values)[1]), 9.0) ||
        !almost_equal(console_calc::scalar_to_double((*geometric_values)[2]), 27.0) ||
        !almost_equal(console_calc::scalar_to_double((*geometric_values)[3]), 81.0)) {
        return false;
    }

    const console_calc::Value generated_repeat = parser.evaluate_value("repeat(2.5, 3)");
    const auto* repeated_values = std::get_if<console_calc::ListValue>(&generated_repeat);
    if (repeated_values == nullptr || repeated_values->size() != 3 ||
        !almost_equal(console_calc::scalar_to_double((*repeated_values)[0]), 2.5) ||
        !almost_equal(console_calc::scalar_to_double((*repeated_values)[1]), 2.5) ||
        !almost_equal(console_calc::scalar_to_double((*repeated_values)[2]), 2.5)) {
        return false;
    }

    const console_calc::Value generated_linspace = parser.evaluate_value("linspace(1, 4, 4)");
    const auto* spaced_values = std::get_if<console_calc::ListValue>(&generated_linspace);
    if (spaced_values == nullptr || spaced_values->size() != 4 ||
        !almost_equal(console_calc::scalar_to_double((*spaced_values)[0]), 1.0) ||
        !almost_equal(console_calc::scalar_to_double((*spaced_values)[1]), 2.0) ||
        !almost_equal(console_calc::scalar_to_double((*spaced_values)[2]), 3.0) ||
        !almost_equal(console_calc::scalar_to_double((*spaced_values)[3]), 4.0)) {
        return false;
    }

    const console_calc::Value generated_powers = parser.evaluate_value("powers(-1, 4)");
    const auto* power_values = std::get_if<console_calc::ListValue>(&generated_powers);
    if (power_values == nullptr || power_values->size() != 4 ||
        !almost_equal(console_calc::scalar_to_double((*power_values)[0]), 1.0) ||
        !almost_equal(console_calc::scalar_to_double((*power_values)[1]), -1.0) ||
        !almost_equal(console_calc::scalar_to_double((*power_values)[2]), 1.0) ||
        !almost_equal(console_calc::scalar_to_double((*power_values)[3]), -1.0)) {
        return false;
    }

    const console_calc::Value added_lists =
        parser.evaluate_value("list_add({2, 3, 4}, {5, 6, 7})");
    const auto* added_values = std::get_if<console_calc::ListValue>(&added_lists);
    if (added_values == nullptr || added_values->size() != 3 ||
        !almost_equal(console_calc::scalar_to_double((*added_values)[0]), 7.0) ||
        !almost_equal(console_calc::scalar_to_double((*added_values)[1]), 9.0) ||
        !almost_equal(console_calc::scalar_to_double((*added_values)[2]), 11.0)) {
        return false;
    }

    const console_calc::Value multiplied_lists =
        parser.evaluate_value("list_mul({2, 3, 4}, {5, 6, 7})");
    const auto* multiplied_values = std::get_if<console_calc::ListValue>(&multiplied_lists);
    if (multiplied_values == nullptr || multiplied_values->size() != 3 ||
        !almost_equal(console_calc::scalar_to_double((*multiplied_values)[0]), 10.0) ||
        !almost_equal(console_calc::scalar_to_double((*multiplied_values)[1]), 18.0) ||
        !almost_equal(console_calc::scalar_to_double((*multiplied_values)[2]), 28.0)) {
        return false;
    }

    const console_calc::Value divided_lists =
        parser.evaluate_value("list_div({8, 9, 10}, {2, 3, 5})");
    const auto* divided_values = std::get_if<console_calc::ListValue>(&divided_lists);
    if (divided_values == nullptr || divided_values->size() != 3 ||
        !almost_equal(console_calc::scalar_to_double((*divided_values)[0]), 4.0) ||
        !almost_equal(console_calc::scalar_to_double((*divided_values)[1]), 3.0) ||
        !almost_equal(console_calc::scalar_to_double((*divided_values)[2]), 2.0)) {
        return false;
    }

    const console_calc::Value subtracted_lists =
        parser.evaluate_value("list_sub({8, 9, 10}, {2, 3, 5})");
    const auto* subtracted_values = std::get_if<console_calc::ListValue>(&subtracted_lists);
    if (subtracted_values == nullptr || subtracted_values->size() != 3 ||
        !almost_equal(console_calc::scalar_to_double((*subtracted_values)[0]), 6.0) ||
        !almost_equal(console_calc::scalar_to_double((*subtracted_values)[1]), 6.0) ||
        !almost_equal(console_calc::scalar_to_double((*subtracted_values)[2]), 5.0)) {
        return false;
    }

    const console_calc::Value reduced_list = parser.evaluate_value("reduce({2, 3, 4}, *)");
    if (!std::holds_alternative<std::int64_t>(reduced_list) ||
        std::get<std::int64_t>(reduced_list) != 24) {
        return false;
    }

    const console_calc::Value absolute_value = parser.evaluate_value("abs(-3)");
    if (!std::holds_alternative<std::int64_t>(absolute_value) ||
        std::get<std::int64_t>(absolute_value) != 3) {
        return false;
    }

    const console_calc::Value square_root_value = parser.evaluate_value("sqrt(9)");
    if (!std::holds_alternative<double>(square_root_value) ||
        !almost_equal(std::get<double>(square_root_value), 3.0)) {
        return false;
    }

    const console_calc::Value position_value = parser.evaluate_value("pos(60, 10)");
    const auto* position = std::get_if<console_calc::PositionValue>(&position_value);
    if (position == nullptr || !almost_equal(position->latitude_deg, 60.0) ||
        !almost_equal(position->longitude_deg, 10.0)) {
        return false;
    }

    if (!almost_equal(parser.evaluate("lat(pos(60, 10))"), 60.0) ||
        !almost_equal(parser.evaluate("lon(pos(60, 10))"), 10.0) ||
        !almost_equal(parser.evaluate("dist(pos(0, 0), pos(0, 1))"), 111319.4907932264, 1e-6) ||
        !almost_equal(parser.evaluate("bearing(pos(0, 0), pos(0, 1))"), 90.0, 1e-9) ||
        !almost_equal(parser.evaluate("lat(br_to_pos(pos(0, 0), 90, 111319.4907932264))"),
                      0.0, 1e-8) ||
        !almost_equal(parser.evaluate("lon(br_to_pos(pos(0, 0), 90, 111319.4907932264))"),
                      1.0, 1e-8)) {
        return false;
    }

    try {
        (void)parser.evaluate("sin({1, 2})");
        return false;
    } catch (const std::invalid_argument&) {
    } catch (const std::exception&) {
        return false;
    }

    try {
        (void)parser.evaluate("pow({2, 3}, 2)");
        return false;
    } catch (const std::invalid_argument&) {
    } catch (const std::exception&) {
        return false;
    }

    try {
        (void)parser.evaluate("lat(1)");
        return false;
    } catch (const std::invalid_argument&) {
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

    const Expression ast = parser.parse("abs(2) + sqrt(9)");
    const auto* root = std::get_if<BinaryExpression>(&ast.node);
    if (root == nullptr || root->op != BinaryOperator::add) {
        return false;
    }

    const auto* lhs = std::get_if<FunctionCall>(&root->left->node);
    if (lhs == nullptr || lhs->function != Function::abs || lhs->arguments.size() != 1) {
        return false;
    }

    const auto* lhs_arg = std::get_if<NumberLiteral>(&lhs->arguments[0]->node);
    if (lhs_arg == nullptr || !almost_equal(lhs_arg->value, 2.0)) {
        return false;
    }

    const auto* rhs = std::get_if<FunctionCall>(&root->right->node);
    if (rhs == nullptr || rhs->function != Function::sqrt || rhs->arguments.size() != 1) {
        return false;
    }

    const auto* rhs_arg = std::get_if<NumberLiteral>(&rhs->arguments[0]->node);
    return rhs_arg != nullptr && almost_equal(rhs_arg->value, 9.0);
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

bool expect_map_expression_ast_shape(console_calc::ExpressionParser& parser) {
    using console_calc::BinaryExpression;
    using console_calc::Expression;
    using console_calc::ListLiteral;
    using console_calc::MapCall;
    using console_calc::NumberLiteral;
    using console_calc::PlaceholderExpression;

    const Expression ast = parser.parse("map({2, 3}, _ + 1)");
    const auto* root = std::get_if<MapCall>(&ast.node);
    if (root == nullptr || root->mapped_expression == nullptr) {
        return false;
    }

    const auto* list = std::get_if<ListLiteral>(&root->list_argument->node);
    if (list == nullptr || list->elements.size() != 2) {
        return false;
    }

    const auto* mapped = std::get_if<BinaryExpression>(&root->mapped_expression->node);
    if (mapped == nullptr) {
        return false;
    }

    return std::holds_alternative<PlaceholderExpression>(mapped->left->node) &&
           std::holds_alternative<NumberLiteral>(mapped->right->node);
}

bool expect_guard_ast_shape(console_calc::ExpressionParser& parser) {
    using console_calc::BinaryExpression;
    using console_calc::BinaryOperator;
    using console_calc::Expression;
    using console_calc::GuardCall;
    using console_calc::NumberLiteral;

    const Expression ast = parser.parse("guard(1 / 0, 5)");
    const auto* root = std::get_if<GuardCall>(&ast.node);
    if (root == nullptr) {
        return false;
    }

    const auto* guarded = std::get_if<BinaryExpression>(&root->guarded_expression->node);
    if (guarded == nullptr || guarded->op != BinaryOperator::divide) {
        return false;
    }

    const auto* fallback = std::get_if<NumberLiteral>(&root->fallback_expression->node);
    return fallback != nullptr && almost_equal(fallback->value, 5.0);
}

bool expect_reduce_ast_shape(console_calc::ExpressionParser& parser) {
    using console_calc::BinaryOperator;
    using console_calc::Expression;
    using console_calc::ListLiteral;
    using console_calc::NumberLiteral;
    using console_calc::ReduceCall;

    const Expression ast = parser.parse("reduce({2, 3}, *)");
    const auto* root = std::get_if<ReduceCall>(&ast.node);
    if (root == nullptr || root->reduction_operator != BinaryOperator::multiply) {
        return false;
    }

    const auto* list = std::get_if<ListLiteral>(&root->list_argument->node);
    if (list == nullptr || list->elements.size() != 2) {
        return false;
    }

    const auto* first = std::get_if<NumberLiteral>(&list->elements[0]->node);
    const auto* second = std::get_if<NumberLiteral>(&list->elements[1]->node);
    return first != nullptr && second != nullptr && almost_equal(first->value, 2.0) &&
           almost_equal(second->value, 3.0);
}

bool expect_timed_loop_ast_shape(console_calc::ExpressionParser& parser) {
    using console_calc::BinaryExpression;
    using console_calc::Expression;
    using console_calc::NumberLiteral;
    using console_calc::TimedLoopCall;

    const Expression ast = parser.parse("timed_loop(1 + 2, 3)");
    const auto* root = std::get_if<TimedLoopCall>(&ast.node);
    if (root == nullptr) {
        return false;
    }

    const auto* loop_expression = std::get_if<BinaryExpression>(&root->loop_expression->node);
    const auto* iteration_count = std::get_if<NumberLiteral>(&root->iteration_count->node);
    return loop_expression != nullptr && iteration_count != nullptr &&
           almost_equal(iteration_count->value, 3.0);
}

bool expect_fill_ast_shape(console_calc::ExpressionParser& parser) {
    using console_calc::Expression;
    using console_calc::FillCall;
    using console_calc::NumberLiteral;

    const Expression ast = parser.parse("fill(1 + 2, 3)");
    const auto* root = std::get_if<FillCall>(&ast.node);
    if (root == nullptr) {
        return false;
    }

    const auto* iteration_count = std::get_if<NumberLiteral>(&root->iteration_count->node);
    return root->fill_expression != nullptr && iteration_count != nullptr &&
           almost_equal(iteration_count->value, 3.0);
}

bool expect_range_ast_shape(console_calc::ExpressionParser& parser) {
    using console_calc::Expression;
    using console_calc::Function;
    using console_calc::FunctionCall;
    using console_calc::NumberLiteral;

    const Expression ast = parser.parse("range(2, 4, 3)");
    const auto* root = std::get_if<FunctionCall>(&ast.node);
    if (root == nullptr || root->function != Function::range || root->arguments.size() != 3) {
        return false;
    }

    const auto* first = std::get_if<NumberLiteral>(&root->arguments[0]->node);
    const auto* second = std::get_if<NumberLiteral>(&root->arguments[1]->node);
    const auto* third = std::get_if<NumberLiteral>(&root->arguments[2]->node);
    return first != nullptr && second != nullptr && third != nullptr &&
           almost_equal(first->value, 2.0) && almost_equal(second->value, 4.0) &&
           almost_equal(third->value, 3.0);
}

bool expect_generator_ast_shapes(console_calc::ExpressionParser& parser) {
    using console_calc::Expression;
    using console_calc::Function;
    using console_calc::FunctionCall;

    const Expression geom_ast = parser.parse("geom(2, 4, 3)");
    const auto* geom_call = std::get_if<FunctionCall>(&geom_ast.node);
    if (geom_call == nullptr || geom_call->function != Function::geom ||
        geom_call->arguments.size() != 3) {
        return false;
    }

    const Expression repeat_ast = parser.parse("repeat(2, 4)");
    const auto* repeat_call = std::get_if<FunctionCall>(&repeat_ast.node);
    if (repeat_call == nullptr || repeat_call->function != Function::repeat ||
        repeat_call->arguments.size() != 2) {
        return false;
    }

    const Expression linspace_ast = parser.parse("linspace(1, 4, 4)");
    const auto* linspace_call = std::get_if<FunctionCall>(&linspace_ast.node);
    if (linspace_call == nullptr || linspace_call->function != Function::linspace ||
        linspace_call->arguments.size() != 3) {
        return false;
    }

    const Expression powers_ast = parser.parse("powers(-1, 4)");
    const auto* powers_call = std::get_if<FunctionCall>(&powers_ast.node);
    if (powers_call == nullptr || powers_call->function != Function::powers ||
        powers_call->arguments.size() != 2) {
        return false;
    }

    const Expression list_mul_ast = parser.parse("list_mul({1, 2}, {3, 4})");
    const auto* list_mul_call = std::get_if<FunctionCall>(&list_mul_ast.node);
    if (list_mul_call == nullptr || list_mul_call->function != Function::list_mul ||
        list_mul_call->arguments.size() != 2) {
        return false;
    }

    const Expression list_add_ast = parser.parse("list_add({1, 2}, {3, 4})");
    const auto* list_add_call = std::get_if<FunctionCall>(&list_add_ast.node);
    if (list_add_call == nullptr || list_add_call->function != Function::list_add ||
        list_add_call->arguments.size() != 2) {
        return false;
    }

    const Expression list_div_ast = parser.parse("list_div({8, 9}, {2, 3})");
    const auto* list_div_call = std::get_if<FunctionCall>(&list_div_ast.node);
    if (list_div_call == nullptr || list_div_call->function != Function::list_div ||
        list_div_call->arguments.size() != 2) {
        return false;
    }

    const Expression list_sub_ast = parser.parse("list_sub({8, 9}, {2, 3})");
    const auto* list_sub_call = std::get_if<FunctionCall>(&list_sub_ast.node);
    return list_sub_call != nullptr && list_sub_call->function == Function::list_sub &&
           list_sub_call->arguments.size() == 2;
}

bool expect_integer_semantics(console_calc::ExpressionParser& parser) {
    const console_calc::Value integer_sum = parser.evaluate_value("1 + 2");
    if (!std::holds_alternative<std::int64_t>(integer_sum) || std::get<std::int64_t>(integer_sum) != 3) {
        return false;
    }

    const console_calc::Value floating_division = parser.evaluate_value("1 / 2");
    if (!std::holds_alternative<double>(floating_division) ||
        !almost_equal(std::get<double>(floating_division), 0.5)) {
        return false;
    }

    const console_calc::Value integer_sum_list = parser.evaluate_value("sum({1, 2, 3})");
    if (!std::holds_alternative<std::int64_t>(integer_sum_list) ||
        std::get<std::int64_t>(integer_sum_list) != 6) {
        return false;
    }

    const console_calc::Value mixed_sum_list = parser.evaluate_value("sum({1, 2.5})");
    if (!std::holds_alternative<double>(mixed_sum_list) ||
        !almost_equal(std::get<double>(mixed_sum_list), 3.5)) {
        return false;
    }

    const console_calc::Value integer_modulo = parser.evaluate_value("7 % 3");
    if (!std::holds_alternative<std::int64_t>(integer_modulo) ||
        std::get<std::int64_t>(integer_modulo) != 1) {
        return false;
    }

    const console_calc::Value floating_modulo = parser.evaluate_value("7.5 % 2");
    if (!std::holds_alternative<double>(floating_modulo) ||
        !almost_equal(std::get<double>(floating_modulo), 1.5)) {
        return false;
    }

    const console_calc::Value integer_length = parser.evaluate_value("len({1, 2, 3})");
    return std::holds_alternative<std::int64_t>(integer_length) &&
           std::get<std::int64_t>(integer_length) == 3;
}

bool expect_timed_loop_behavior(console_calc::ExpressionParser& parser) {
    const console_calc::Value elapsed = parser.evaluate_value("timed_loop(1 + 2, 3)");
    if (!std::holds_alternative<double>(elapsed) || std::get<double>(elapsed) < 0.0 ||
        !std::isfinite(std::get<double>(elapsed))) {
        return false;
    }

    const console_calc::Value zero_elapsed = parser.evaluate_value("timed_loop(1 + 2, 0)");
    if (!std::holds_alternative<double>(zero_elapsed) || std::get<double>(zero_elapsed) < 0.0 ||
        !std::isfinite(std::get<double>(zero_elapsed))) {
        return false;
    }

    try {
        (void)parser.evaluate("timed_loop(1 + 2, -1)");
        return false;
    } catch (const std::invalid_argument&) {
    } catch (const std::exception&) {
        return false;
    }

    return true;
}

bool expect_fill_behavior(console_calc::ExpressionParser& parser) {
    const console_calc::Value filled = parser.evaluate_value("fill(1 + 2, 3)");
    if (!std::holds_alternative<console_calc::ListValue>(filled)) {
        return false;
    }

    const auto& values = std::get<console_calc::ListValue>(filled);
    if (values.size() != 3) {
        return false;
    }

    return std::holds_alternative<std::int64_t>(values[0]) &&
           std::get<std::int64_t>(values[0]) == 3 &&
           std::holds_alternative<std::int64_t>(values[1]) &&
           std::get<std::int64_t>(values[1]) == 3 &&
           std::holds_alternative<std::int64_t>(values[2]) &&
           std::get<std::int64_t>(values[2]) == 3;
}

bool expect_rand_behavior(console_calc::ExpressionParser& parser) {
    const console_calc::Value default_random = parser.evaluate_value("rand()");
    if (!std::holds_alternative<double>(default_random) ||
        std::get<double>(default_random) < 0.0 ||
        std::get<double>(default_random) >= 1.0) {
        return false;
    }

    const console_calc::Value bounded_random = parser.evaluate_value("rand(5)");
    if (!std::holds_alternative<double>(bounded_random) ||
        std::get<double>(bounded_random) < 0.0 ||
        std::get<double>(bounded_random) >= 5.0) {
        return false;
    }

    const console_calc::Value ranged_random = parser.evaluate_value("rand(2, 5)");
    if (!std::holds_alternative<double>(ranged_random) ||
        std::get<double>(ranged_random) < 2.0 ||
        std::get<double>(ranged_random) >= 5.0) {
        return false;
    }

    try {
        (void)parser.evaluate("rand(1, 1)");
        return false;
    } catch (const std::invalid_argument&) {
    } catch (const std::exception&) {
        return false;
    }

    try {
        (void)parser.evaluate("rand(5, 2)");
        return false;
    } catch (const std::invalid_argument&) {
    } catch (const std::exception&) {
        return false;
    }

    return true;
}

bool expect_function_signature_errors(console_calc::ExpressionParser& parser) {
    try {
        (void)parser.parse("pow(2)");
        return false;
    } catch (const std::invalid_argument& error) {
        if (std::string(error.what()) != "function 'pow' expects pow(x, y)") {
            return false;
        }
    } catch (const std::exception&) {
        return false;
    }

    try {
        (void)parser.parse("range(1)");
        return false;
    } catch (const std::invalid_argument& error) {
        if (std::string(error.what()) != "function 'range' expects range(start, count[, step])") {
            return false;
        }
    } catch (const std::exception&) {
        return false;
    }

    try {
        (void)parser.parse("guard(1 / 0)");
        return false;
    } catch (const std::invalid_argument& error) {
        if (std::string(error.what()) != "function 'guard' expects guard(expr, fallback)") {
            return false;
        }
    } catch (const std::exception&) {
        return false;
    }

    try {
        (void)parser.parse("timed_loop(1)");
        return false;
    } catch (const std::invalid_argument& error) {
        if (std::string(error.what()) !=
            "function 'timed_loop' expects timed_loop(expr, count)") {
            return false;
        }
    } catch (const std::exception&) {
        return false;
    }

    try {
        (void)parser.parse("fill(1)");
        return false;
    } catch (const std::invalid_argument& error) {
        if (std::string(error.what()) != "function 'fill' expects fill(expr, count)") {
            return false;
        }
    } catch (const std::exception&) {
        return false;
    }

    try {
        (void)parser.parse("rand(1, 2, 3)");
        return false;
    } catch (const std::invalid_argument& error) {
        if (std::string(error.what()) != "function 'rand' expects rand([min, max])") {
            return false;
        }
    } catch (const std::exception&) {
        return false;
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

    if (!expect_map_expression_ast_shape(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_guard_ast_shape(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_reduce_ast_shape(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_timed_loop_ast_shape(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_fill_ast_shape(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_range_ast_shape(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_generator_ast_shapes(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_integer_semantics(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_timed_loop_behavior(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_fill_behavior(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_rand_behavior(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_function_signature_errors(parser)) {
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
