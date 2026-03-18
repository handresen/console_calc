#include "console_calc/builtin_function.h"

namespace console_calc {

std::optional<Function> parse_builtin_function(std::string_view name) {
    if (name == "sin") {
        return Function::sin;
    }
    if (name == "cos") {
        return Function::cos;
    }
    if (name == "tan") {
        return Function::tan;
    }
    if (name == "sind") {
        return Function::sind;
    }
    if (name == "cosd") {
        return Function::cosd;
    }
    if (name == "tand") {
        return Function::tand;
    }
    if (name == "pow") {
        return Function::pow;
    }
    if (name == "sum") {
        return Function::sum;
    }

    return std::nullopt;
}

std::string_view builtin_function_name(Function function) {
    switch (function) {
    case Function::sin:
        return "sin";
    case Function::cos:
        return "cos";
    case Function::tan:
        return "tan";
    case Function::sind:
        return "sind";
    case Function::cosd:
        return "cosd";
    case Function::tand:
        return "tand";
    case Function::pow:
        return "pow";
    case Function::sum:
        return "sum";
    }

    return "";
}

std::size_t builtin_function_arity(Function function) {
    switch (function) {
    case Function::sin:
    case Function::cos:
    case Function::tan:
    case Function::sind:
    case Function::cosd:
    case Function::tand:
    case Function::sum:
        return 1;
    case Function::pow:
        return 2;
    }

    return 0;
}

bool is_builtin_function_name(std::string_view name) {
    return parse_builtin_function(name).has_value();
}

}  // namespace console_calc
