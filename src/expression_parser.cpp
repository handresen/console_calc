#include "console_calc/expression_parser.h"

#include "expression_ast_parser.h"
#include "expression_evaluator.h"

namespace console_calc {

Expression ExpressionParser::parse(const std::string& expression) const {
    return parse_expression(expression);
}

double ExpressionParser::evaluate(const std::string& expression) const {
    return evaluate_expression(parse(expression));
}

}  // namespace console_calc
