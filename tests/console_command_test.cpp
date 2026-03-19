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
           console_calc::classify_console_command("stack_depth(8)").kind ==
               ConsoleCommandKind::expression &&
           console_calc::classify_console_command("x:1").kind ==
               ConsoleCommandKind::assignment &&
           console_calc::classify_console_command("sin(pi)").kind ==
               ConsoleCommandKind::expression;
}

bool expect_builtin_function_listing() {
    return console_calc::format_builtin_function_listing(console_calc::builtin_functions()) ==
           "Scalar functions\n"
           "  abs(x)                            absolute value\n"
           "  cos(x)                            cosine in radians\n"
           "  cosd(x)                           cosine in degrees\n"
           "  guard(expr, fallback)             use fallback when expr evaluation fails\n"
           "  pow(x, y)                         power\n"
           "  sin(x)                            sine in radians\n"
           "  sind(x)                           sine in degrees\n"
           "  sqrt(x)                           square root\n"
           "  tan(x)                            tangent in radians\n"
           "  tand(x)                           tangent in degrees\n"
           "\n"
           "List functions\n"
           "  avg(list)                         average of list elements\n"
           "  drop(n, list)                     drop first n list elements\n"
           "  first(n, list)                    first n list elements\n"
           "  len(list)                         list length\n"
           "  list_add(a, b)                    add matching list elements\n"
           "  list_div(a, b)                    divide matching list elements\n"
           "  list_mul(a, b)                    multiply matching list elements\n"
           "  list_sub(a, b)                    subtract matching list elements\n"
           "  map(list, expr)                   map inline expression over list\n"
           "  max(list)                         maximum list element\n"
           "  min(list)                         minimum list element\n"
           "  product(list)                     product of list elements\n"
           "  reduce(list, op)                  reduce list with binary operator\n"
           "  sum(list)                         sum list elements\n"
           "\n"
           "List generation functions\n"
           "  geom(start, count[, ratio])       generate geometric series from start\n"
           "  linspace(start, stop, count)      generate evenly spaced values over interval\n"
           "  powers(base, count[, start_exp])  generate successive integer powers\n"
           "  range(start, count[, step])       generate linear series from start\n"
           "  repeat(value, count)              repeat value count times\n";
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

bool expect_structured_listing_views() {
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
    const std::vector<console_calc::Value> stack{
        std::int64_t{2},
        console_calc::ListValue{console_calc::ScalarValue{1}, console_calc::ScalarValue{2}},
    };

    const auto stack_views = console_calc::stack_entry_views(stack);
    const auto definition_list = console_calc::definition_views(definitions);
    const auto constant_list = console_calc::constant_views(constants);
    const auto function_list =
        console_calc::builtin_function_views(console_calc::builtin_functions());

    return stack_views.size() == 2 && stack_views[0].level == 0 &&
           std::holds_alternative<std::int64_t>(stack_views[0].value) &&
           std::get<std::int64_t>(stack_views[0].value) == 2 && stack_views[1].level == 1 &&
           std::holds_alternative<console_calc::ListValue>(stack_views[1].value) &&
           definition_list.size() == 3 && definition_list[0].name == "sx" &&
           definition_list[0].expression == "sin(x)" && definition_list[2].name == "x" &&
           constant_list.size() == 3 && constant_list[0].name == "e" &&
           std::holds_alternative<double>(constant_list[0].value) &&
           std::get<double>(constant_list[0].value) == 2.7182818284590451 &&
           !function_list.empty() && function_list.front().name == "abs" &&
           function_list.front().signature == "abs(x)" &&
           function_list.front().category == console_calc::BuiltinFunctionCategory::scalar &&
           function_list.front().summary == "absolute value";
}

bool expect_builtin_function_metadata() {
    const auto sum_info = console_calc::builtin_function_info(console_calc::Function::sum);
    const auto abs_info = console_calc::builtin_function_info(console_calc::Function::abs);
    const auto sqrt_info = console_calc::builtin_function_info(console_calc::Function::sqrt);
    const auto list_add_info = console_calc::builtin_function_info(console_calc::Function::list_add);
    const auto list_div_info = console_calc::builtin_function_info(console_calc::Function::list_div);
    const auto list_mul_info = console_calc::builtin_function_info(console_calc::Function::list_mul);
    const auto list_sub_info = console_calc::builtin_function_info(console_calc::Function::list_sub);
    const auto guard_info = console_calc::builtin_function_info(console_calc::Function::guard);
    const auto reduce_info = console_calc::builtin_function_info(console_calc::Function::reduce);
    const auto map_info = console_calc::builtin_function_info(console_calc::Function::map);
    const auto range_info = console_calc::builtin_function_info(console_calc::Function::range);
    const auto geom_info = console_calc::builtin_function_info(console_calc::Function::geom);
    const auto repeat_info = console_calc::builtin_function_info(console_calc::Function::repeat);
    const auto linspace_info = console_calc::builtin_function_info(console_calc::Function::linspace);
    const auto powers_info = console_calc::builtin_function_info(console_calc::Function::powers);
    return abs_info.name == "abs" && abs_info.min_arity == 1 && abs_info.max_arity == 1 &&
           abs_info.category == console_calc::BuiltinFunctionCategory::scalar &&
           abs_info.summary == "absolute value" && abs_info.mappable &&
           sqrt_info.name == "sqrt" && sqrt_info.min_arity == 1 && sqrt_info.max_arity == 1 &&
           sqrt_info.category == console_calc::BuiltinFunctionCategory::scalar &&
           sqrt_info.summary == "square root" && sqrt_info.mappable &&
           sum_info.name == "sum" && sum_info.min_arity == 1 && sum_info.max_arity == 1 &&
           sum_info.category == console_calc::BuiltinFunctionCategory::list &&
           sum_info.summary == "sum list elements" && !sum_info.mappable &&
           list_add_info.name == "list_add" && list_add_info.min_arity == 2 &&
           list_add_info.max_arity == 2 &&
           list_add_info.category == console_calc::BuiltinFunctionCategory::list &&
           list_add_info.summary == "add matching list elements" &&
           list_div_info.name == "list_div" && list_div_info.min_arity == 2 &&
           list_div_info.max_arity == 2 &&
           list_div_info.category == console_calc::BuiltinFunctionCategory::list &&
           list_div_info.summary == "divide matching list elements" &&
           list_mul_info.name == "list_mul" && list_mul_info.min_arity == 2 &&
           list_mul_info.max_arity == 2 &&
           list_mul_info.category == console_calc::BuiltinFunctionCategory::list &&
           list_mul_info.summary == "multiply matching list elements" &&
           list_sub_info.name == "list_sub" && list_sub_info.min_arity == 2 &&
           list_sub_info.max_arity == 2 &&
           list_sub_info.category == console_calc::BuiltinFunctionCategory::list &&
           list_sub_info.summary == "subtract matching list elements" &&
           guard_info.name == "guard" && guard_info.min_arity == 2 &&
           guard_info.max_arity == 2 &&
           guard_info.category == console_calc::BuiltinFunctionCategory::scalar &&
           guard_info.signature == "guard(expr, fallback)" &&
           guard_info.summary == "use fallback when expr evaluation fails" &&
           reduce_info.name == "reduce" && reduce_info.min_arity == 2 &&
           reduce_info.max_arity == 2 &&
           reduce_info.category == console_calc::BuiltinFunctionCategory::list &&
           reduce_info.summary == "reduce list with binary operator" &&
           map_info.name == "map" && map_info.min_arity == 2 && map_info.max_arity == 2 &&
           map_info.category == console_calc::BuiltinFunctionCategory::list &&
           map_info.signature == "map(list, expr)" &&
           map_info.summary == "map inline expression over list" && !map_info.mappable &&
           range_info.name == "range" && range_info.min_arity == 2 && range_info.max_arity == 3 &&
           range_info.category == console_calc::BuiltinFunctionCategory::list_generation &&
           range_info.summary == "generate linear series from start" &&
           range_info.signature == "range(start, count[, step])" &&
           geom_info.min_arity == 2 && geom_info.max_arity == 3 &&
           geom_info.category == console_calc::BuiltinFunctionCategory::list_generation &&
           repeat_info.min_arity == 2 && repeat_info.max_arity == 2 &&
           repeat_info.category == console_calc::BuiltinFunctionCategory::list_generation &&
           linspace_info.min_arity == 3 && linspace_info.max_arity == 3 &&
           linspace_info.category == console_calc::BuiltinFunctionCategory::list_generation &&
           powers_info.min_arity == 2 && powers_info.max_arity == 3 &&
           powers_info.category == console_calc::BuiltinFunctionCategory::list_generation;
}

bool expect_builtin_function_helpers() {
    return console_calc::is_scalar_function(console_calc::Function::sin) &&
           console_calc::is_scalar_function(console_calc::Function::abs) &&
           console_calc::is_scalar_function(console_calc::Function::guard) &&
           !console_calc::is_scalar_function(console_calc::Function::sum) &&
           console_calc::is_list_function(console_calc::Function::sum) &&
           console_calc::is_list_function(console_calc::Function::list_add) &&
           console_calc::is_list_function(console_calc::Function::range) &&
           !console_calc::is_list_function(console_calc::Function::pow) &&
           console_calc::is_unary_scalar_function(console_calc::Function::sin) &&
           console_calc::is_unary_scalar_function(console_calc::Function::abs) &&
           console_calc::is_unary_scalar_function(console_calc::Function::sqrt) &&
           !console_calc::is_unary_scalar_function(console_calc::Function::pow) &&
           console_calc::is_mappable_unary_scalar_function(console_calc::Function::sin) &&
           console_calc::is_mappable_unary_scalar_function(console_calc::Function::abs) &&
           console_calc::is_mappable_unary_scalar_function(console_calc::Function::sqrt) &&
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
               "map(vals, sin(_) + _)", constants, definitions, std::nullopt) ==
               "map({1, 2, 3}, sin(_) + _)" &&
           console_calc::expand_expression_identifiers(
               "0x10 + 5", constants, definitions, std::nullopt) ==
               "0x10 + 5" &&
           console_calc::expand_expression_identifiers(
               "0b1010 + 1", constants, definitions, std::nullopt) ==
               "0b1010 + 1";
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

    if (!expect_structured_listing_views()) {
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
