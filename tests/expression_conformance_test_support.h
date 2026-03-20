#pragma once

#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>

#include "console_calc/expression_parser.h"
#include "console_calc/scalar_value.h"

namespace console_calc::test {

inline bool almost_equal(double lhs, double rhs) {
    return std::fabs(lhs - rhs) < 1e-12;
}

inline bool almost_equal(double lhs, double rhs, double tolerance) {
    return std::fabs(lhs - rhs) < tolerance;
}

inline bool almost_equal(const console_calc::ScalarValue& lhs, double rhs) {
    return almost_equal(console_calc::scalar_to_double(lhs), rhs);
}

inline bool expect_value(console_calc::ExpressionParser& parser, const std::string& expression,
                         double expected) {
    try {
        return almost_equal(parser.evaluate(expression), expected);
    } catch (const std::exception&) {
        return false;
    }
}

inline bool expect_invalid(console_calc::ExpressionParser& parser,
                           const std::string& expression) {
    try {
        (void)parser.evaluate(expression);
        return false;
    } catch (const std::invalid_argument&) {
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

inline bool run_conformance_check(std::string_view label, const std::function<bool()>& check) {
    try {
        if (check()) {
            return true;
        }
        std::cerr << "conformance check failed: " << label << '\n';
        return false;
    } catch (const std::exception& error) {
        std::cerr << "conformance check threw in " << label << ": " << error.what() << '\n';
        return false;
    }
}

}  // namespace console_calc::test
