#include <iostream>
#include <string>

#include "console_calc/expression_parser.h"

int main(int argc, char* argv[]) {
    console_calc::ExpressionParser parser;

    if (argc < 2) {
        std::cerr << "usage: console_calc <expression>\n";
        return 1;
    }

    const std::string expression = argv[1];
    const double result = parser.evaluate(expression);
    std::cout << result << '\n';

    return 0;
}
