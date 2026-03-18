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
           console_calc::classify_console_command("dec").kind ==
               ConsoleCommandKind::display_decimal &&
           console_calc::classify_console_command("hex").kind ==
               ConsoleCommandKind::display_hexadecimal &&
           console_calc::classify_console_command("bin").kind ==
               ConsoleCommandKind::display_binary &&
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
           "  cos/1      cosine in radians\n"
           "  cosd/1     cosine in degrees\n"
           "  pow/2      power\n"
           "  sin/1      sine in radians\n"
           "  sind/1     sine in degrees\n"
           "  tan/1      tangent in radians\n"
           "  tand/1     tangent in degrees\n"
           "\n"
           "List functions\n"
           "  avg/1      average of list elements\n"
           "  drop/2     drop first n list elements\n"
           "  first/2    first n list elements\n"
           "  len/1      list length\n"
           "  map/2      map unary scalar builtin over list\n"
           "  max/1      maximum list element\n"
           "  min/1      minimum list element\n"
           "  product/1  product of list elements\n"
           "  sum/1      sum list elements\n";
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

bool expect_builtin_function_helpers() {
    return console_calc::is_scalar_function(console_calc::Function::sin) &&
           !console_calc::is_scalar_function(console_calc::Function::sum) &&
           console_calc::is_list_function(console_calc::Function::sum) &&
           !console_calc::is_list_function(console_calc::Function::pow) &&
           console_calc::is_unary_scalar_function(console_calc::Function::sin) &&
           !console_calc::is_unary_scalar_function(console_calc::Function::pow) &&
           console_calc::is_mappable_unary_scalar_function(console_calc::Function::sin) &&
           !console_calc::is_mappable_unary_scalar_function(console_calc::Function::pow) &&
           !console_calc::is_mappable_unary_scalar_function(console_calc::Function::sum);
}

bool expect_expression_identifier_expansion() {
    const console_calc::ConstantTable constants{
        {"pi", 3.14159265358979323846},
    };
    const console_calc::DefinitionTable definitions{
        {"x", {"pi + 1"}},
        {"vals", {"{1, 2, 3}"}},
    };

    return console_calc::expand_expression_identifiers(
               "sin(x)", constants, definitions, std::nullopt) ==
               "sin((3.1415926535897931 + 1))" &&
           console_calc::expand_expression_identifiers(
               "sum(vals)", constants, definitions, std::nullopt) ==
               "sum({1, 2, 3})" &&
           console_calc::expand_expression_identifiers(
               "map(vals, sin)", constants, definitions, std::nullopt) ==
               "map({1, 2, 3}, sin)" &&
           console_calc::expand_expression_identifiers(
               "map({1, 2}, sind)", constants, definitions, std::nullopt) ==
               "map({1, 2}, sind)";
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

    if (!expect_builtin_function_helpers()) {
        return EXIT_FAILURE;
    }

    if (!expect_expression_identifier_expansion()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
