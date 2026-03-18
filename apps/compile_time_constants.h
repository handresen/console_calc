#pragma once

#include "expression_environment.h"

namespace console_calc {

[[nodiscard]] inline ConstantTable builtin_constant_table() {
    return {
        {"pi", 3.14159265358979323846},
        {"e", 2.71828182845904523536},
        {"tau", 6.28318530717958647692},
    };
}

}  // namespace console_calc
