#include "console_calc/builtin_function.h"

#include <array>

namespace console_calc {

namespace {

constexpr std::array k_builtin_functions = {
    Function::sin,  Function::cos,    Function::tan,   Function::sind, Function::cosd,
    Function::tand, Function::pow,    Function::sum,   Function::len,  Function::product,
    Function::avg,  Function::min,    Function::max,   Function::first, Function::drop,
};

}  // namespace

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
    if (name == "len") {
        return Function::len;
    }
    if (name == "product") {
        return Function::product;
    }
    if (name == "avg") {
        return Function::avg;
    }
    if (name == "min") {
        return Function::min;
    }
    if (name == "max") {
        return Function::max;
    }
    if (name == "first") {
        return Function::first;
    }
    if (name == "drop") {
        return Function::drop;
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
    case Function::len:
        return "len";
    case Function::product:
        return "product";
    case Function::avg:
        return "avg";
    case Function::min:
        return "min";
    case Function::max:
        return "max";
    case Function::first:
        return "first";
    case Function::drop:
        return "drop";
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
    case Function::len:
    case Function::product:
    case Function::avg:
    case Function::min:
    case Function::max:
        return 1;
    case Function::pow:
    case Function::first:
    case Function::drop:
        return 2;
    }

    return 0;
}

bool is_builtin_function_name(std::string_view name) {
    return parse_builtin_function(name).has_value();
}

std::span<const Function> builtin_functions() {
    return k_builtin_functions;
}

bool is_list_function(Function function) {
    switch (function) {
    case Function::sum:
    case Function::len:
    case Function::product:
    case Function::avg:
    case Function::min:
    case Function::max:
    case Function::first:
    case Function::drop:
        return true;
    case Function::sin:
    case Function::cos:
    case Function::tan:
    case Function::sind:
    case Function::cosd:
    case Function::tand:
    case Function::pow:
        return false;
    }

    return false;
}

}  // namespace console_calc
