#include <cmath>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <sstream>
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

struct ExpressionCase {
    std::string expression;
    bool expect_invalid = false;
    double expected_value = 0.0;
};

[[nodiscard]] std::string trim(const std::string& text) {
    const std::size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }

    const std::size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

[[nodiscard]] std::vector<ExpressionCase> load_expression_cases() {
    const std::string path = std::string(CONSOLE_CALC_TEST_DATA_DIR) + "/expression_cases.txt";
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open expression test data file");
    }

    std::vector<ExpressionCase> cases;
    std::string line;
    while (std::getline(input, line)) {
        const std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        if (trimmed.size() < 3 || trimmed.front() != '"') {
            throw std::runtime_error("invalid test data line");
        }

        const std::size_t closing_quote = trimmed.find('"', 1);
        if (closing_quote == std::string::npos) {
            throw std::runtime_error("missing closing quote in test data");
        }

        const std::size_t comma = trimmed.find(',', closing_quote + 1);
        if (comma == std::string::npos) {
            throw std::runtime_error("missing comma in test data");
        }

        ExpressionCase expression_case;
        expression_case.expression = trimmed.substr(1, closing_quote - 1);

        const std::string expected = trim(trimmed.substr(comma + 1));
        if (expected == "INVALID") {
            expression_case.expect_invalid = true;
        } else {
            std::istringstream parser(expected);
            parser >> expression_case.expected_value;
            if (!parser || !parser.eof()) {
                throw std::runtime_error("invalid expected value in test data");
            }
        }

        cases.push_back(std::move(expression_case));
    }

    return cases;
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

}  // namespace

int main() {
    console_calc::ExpressionParser parser;

    if (!expect_ast_shape(parser)) {
        return EXIT_FAILURE;
    }

    if (!expect_parenthesized_ast_shape(parser)) {
        return EXIT_FAILURE;
    }

    const std::vector<ExpressionCase> expression_cases = load_expression_cases();
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
