#pragma once

#include "console_calc/expression_parser.h"

namespace console_calc::test {

bool expect_expression_value_api(ExpressionParser& parser);
bool expect_expression_ast_shape(ExpressionParser& parser);
bool expect_expression_semantics(ExpressionParser& parser);
bool expect_expression_function_signature_errors(ExpressionParser& parser);

}  // namespace console_calc::test
