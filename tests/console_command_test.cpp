#include <cstdlib>
#include <string>

#include "console_command.h"
#include "console_listing.h"
#include "expression_environment.h"
#include "console_calc/builtin_function.h"

namespace {

bool expect_command_classification() {
    using console_calc::ConsoleCommandKind;

    return console_calc::classify_console_command("q").kind == ConsoleCommandKind::quit &&
           console_calc::classify_console_command("s").kind == ConsoleCommandKind::list_stack &&
           console_calc::classify_console_command("vars").kind ==
               ConsoleCommandKind::list_variables &&
           console_calc::classify_console_command("consts").kind ==
               ConsoleCommandKind::list_constants &&
           console_calc::classify_console_command("funcs").kind ==
               ConsoleCommandKind::list_functions &&
           console_calc::classify_console_command("+").kind ==
               ConsoleCommandKind::stack_operator &&
           console_calc::classify_console_command("x:1").kind ==
               ConsoleCommandKind::assignment &&
           console_calc::classify_console_command("sin(pi)").kind ==
               ConsoleCommandKind::expression;
}

bool expect_builtin_function_listing() {
    return console_calc::format_builtin_function_listing(console_calc::builtin_functions()) ==
           "Scalar functions\n"
           "cos/1\n"
           "cosd/1\n"
           "pow/2\n"
           "sin/1\n"
           "sind/1\n"
           "tan/1\n"
           "tand/1\n"
           "List functions\n"
           "avg/1\n"
            "drop/2\n"
            "first/2\n"
            "len/1\n"
            "map/2\n"
            "max/1\n"
            "min/1\n"
            "product/1\n"
           "sum/1\n";
}

bool expect_constant_and_definition_listing() {
    const console_calc::ConstantTable constants{
        {"tau", 6.2831853071795862},
        {"e", 2.7182818284590451},
        {"pi", 3.1415926535897931},
    };
    const console_calc::DefinitionTable definitions{
        {"sx", {"sin(x)"}},
        {"vals", {"{1, 2, 3}"}},
        {"x", {"pi+1"}},
    };

    return console_calc::format_constant_listing(constants) ==
               "e:2.7182818284590451\n"
               "pi:3.1415926535897931\n"
               "tau:6.2831853071795862\n" &&
           console_calc::format_definition_listing(definitions) ==
               "sx:sin(x)\n"
               "vals:{1, 2, 3}\n"
               "x:pi+1\n";
}

bool expect_builtin_function_metadata() {
    const auto sum_info = console_calc::builtin_function_info(console_calc::Function::sum);
    const auto map_info = console_calc::builtin_function_info(console_calc::Function::map);
    return sum_info.name == "sum" && sum_info.arity == 1 &&
           sum_info.category == console_calc::BuiltinFunctionCategory::list &&
           sum_info.summary == "sum list elements" && !sum_info.mappable &&
           map_info.name == "map" && map_info.arity == 2 &&
           map_info.category == console_calc::BuiltinFunctionCategory::list &&
           map_info.summary == "map unary scalar builtin over list" && !map_info.mappable;
}

}  // namespace

int main() {
    if (!expect_command_classification()) {
        return EXIT_FAILURE;
    }

    if (!expect_builtin_function_listing()) {
        return EXIT_FAILURE;
    }

    if (!expect_constant_and_definition_listing()) {
        return EXIT_FAILURE;
    }

    if (!expect_builtin_function_metadata()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
