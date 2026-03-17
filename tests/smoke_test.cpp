#include <cstdlib>

#include "console_calc/expression_parser.h"

int main() {
    console_calc::ExpressionParser parser;
    (void)parser.evaluate("1+1");
    return EXIT_SUCCESS;
}
