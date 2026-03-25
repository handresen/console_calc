#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "console_assignment.h"
#include "console_command.h"
#include "console_listing.h"
#include "compile_time_constants.h"
#include "expression_environment.h"
#include "console_calc/builtin_function.h"
#include "console_calc/special_form.h"

namespace {

bool expect_equal(std::string_view context, std::string_view actual, std::string_view expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << context << "\nexpected: " << expected << "\nactual:   " << actual << '\n';
    return false;
}

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
    const std::string listing =
        console_calc::format_function_listing(console_calc::builtin_functions(),
                                              console_calc::special_forms());
    return listing.find("Scalar functions\n") != std::string::npos &&
           listing.find("\nPosition functions\n") != std::string::npos &&
           listing.find("\nList functions\n") != std::string::npos &&
           listing.find("\nList generation functions\n") != std::string::npos &&
           listing.find("abs(x)") != std::string::npos &&
           listing.find("to_list(poslist)") != std::string::npos &&
           listing.find("to_poslist(list)") != std::string::npos &&
           listing.find("map(list, expr[, start[, step[, count]]])") != std::string::npos &&
           listing.find("map_at(list, expr[, start[, step[, count]]])") != std::string::npos &&
           listing.find("list_where(list, expr)") != std::string::npos &&
           listing.find("fill(expr, count)") != std::string::npos;
}

bool expect_constant_and_definition_listing() {
    const console_calc::ConstantTable constants{
        {"m.pi", 3.1415926535897931},
        {"tau", 6.2831853071795862},
        {"e", 2.7182818284590451},
        {"pi", 3.1415926535897931},
        {"c.deg", 0.017453292519943295},
        {"ph.c", 299792458.0},
    };
    const console_calc::DefinitionTable definitions{
        {"sx", console_calc::make_value_definition("sin(x)")},
        {"vals", console_calc::make_value_definition("{1, 2, 3}")},
        {"x", console_calc::make_value_definition("pi+1")},
    };

    return console_calc::format_constant_listing(constants) ==
               "[root]\n"
               "  e:2.7182818284590451\n"
               "  pi:3.1415926535897931\n"
               "  tau:6.2831853071795862\n"
               "\n"
               "[m]\n"
               "  m.pi:3.1415926535897931\n"
               "\n"
               "[c]\n"
               "  c.deg:0.017453292519943295\n"
               "\n"
               "[ph]\n"
               "  ph.c:299792458\n" &&
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
        {"sx", console_calc::make_value_definition("sin(x)")},
        {"vals", console_calc::make_value_definition("{1, 2, 3}")},
        {"x", console_calc::make_value_definition("pi+1")},
    };
    const std::vector<console_calc::Value> stack{
        std::int64_t{2},
        console_calc::ListValue{console_calc::ScalarValue{1}, console_calc::ScalarValue{2}},
    };

    const auto stack_views = console_calc::stack_entry_views(stack);
    const auto definition_list = console_calc::definition_views(definitions);
    const auto constant_list = console_calc::constant_views(constants);
    const auto function_list = console_calc::function_views(console_calc::builtin_functions(),
                                                            console_calc::special_forms());

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
    const auto and_info = console_calc::builtin_function_info(console_calc::Function::bit_and);
    const auto or_info = console_calc::builtin_function_info(console_calc::Function::bit_or);
    const auto xor_info = console_calc::builtin_function_info(console_calc::Function::bit_xor);
    const auto nand_info = console_calc::builtin_function_info(console_calc::Function::bit_nand);
    const auto nor_info = console_calc::builtin_function_info(console_calc::Function::bit_nor);
    const auto shl_info = console_calc::builtin_function_info(console_calc::Function::shl);
    const auto shr_info = console_calc::builtin_function_info(console_calc::Function::shr);
    const auto sqrt_info = console_calc::builtin_function_info(console_calc::Function::sqrt);
    const auto rand_info = console_calc::builtin_function_info(console_calc::Function::rand);
    const auto list_add_info = console_calc::builtin_function_info(console_calc::Function::list_add);
    const auto list_div_info = console_calc::builtin_function_info(console_calc::Function::list_div);
    const auto list_mul_info = console_calc::builtin_function_info(console_calc::Function::list_mul);
    const auto list_sub_info = console_calc::builtin_function_info(console_calc::Function::list_sub);
    const auto first_info = console_calc::builtin_function_info(console_calc::Function::first);
    const auto drop_info = console_calc::builtin_function_info(console_calc::Function::drop);
    const auto guard_info = console_calc::special_form_info(console_calc::Function::guard);
    const auto pos_info = console_calc::builtin_function_info(console_calc::Function::pos);
    const auto lat_info = console_calc::builtin_function_info(console_calc::Function::lat);
    const auto lon_info = console_calc::builtin_function_info(console_calc::Function::lon);
    const auto to_list_info = console_calc::builtin_function_info(console_calc::Function::to_list);
    const auto to_poslist_info =
        console_calc::builtin_function_info(console_calc::Function::to_poslist);
    const auto densify_path_info =
        console_calc::builtin_function_info(console_calc::Function::densify_path);
    const auto offset_path_info =
        console_calc::builtin_function_info(console_calc::Function::offset_path);
    const auto simplify_path_info =
        console_calc::builtin_function_info(console_calc::Function::simplify_path);
    const auto compress_path_info =
        console_calc::builtin_function_info(console_calc::Function::compress_path);
    const auto dist_info = console_calc::builtin_function_info(console_calc::Function::dist);
    const auto bearing_info = console_calc::builtin_function_info(console_calc::Function::bearing);
    const auto br_to_pos_info = console_calc::builtin_function_info(console_calc::Function::br_to_pos);
    const auto reduce_info = console_calc::special_form_info(console_calc::Function::reduce);
    const auto timed_loop_info =
        console_calc::special_form_info(console_calc::Function::timed_loop);
    const auto fill_info = console_calc::special_form_info(console_calc::Function::fill);
    const auto map_info = console_calc::special_form_info(console_calc::Function::map);
    const auto map_at_info = console_calc::special_form_info(console_calc::Function::map_at);
    const auto list_where_info =
        console_calc::special_form_info(console_calc::Function::list_where);
    const auto range_info = console_calc::builtin_function_info(console_calc::Function::range);
    const auto geom_info = console_calc::builtin_function_info(console_calc::Function::geom);
    const auto repeat_info = console_calc::builtin_function_info(console_calc::Function::repeat);
    const auto linspace_info = console_calc::builtin_function_info(console_calc::Function::linspace);
    const auto powers_info = console_calc::builtin_function_info(console_calc::Function::powers);
    return abs_info.name == "abs" && abs_info.min_arity == 1 && abs_info.max_arity == 1 &&
           abs_info.category == console_calc::BuiltinFunctionCategory::scalar &&
           abs_info.summary == "absolute value" && abs_info.mappable &&
           and_info.name == "and" && and_info.signature == "and(a, b)" &&
           and_info.summary == "bitwise and" &&
           or_info.name == "or" && or_info.signature == "or(a, b)" &&
           or_info.summary == "bitwise or" &&
           xor_info.name == "xor" && xor_info.signature == "xor(a, b)" &&
           xor_info.summary == "bitwise exclusive or" &&
           nand_info.name == "nand" && nand_info.signature == "nand(a, b)" &&
           nand_info.summary == "bitwise not-and" &&
           nor_info.name == "nor" && nor_info.signature == "nor(a, b)" &&
           nor_info.summary == "bitwise not-or" &&
           shl_info.name == "shl" && shl_info.signature == "shl(x, n)" &&
           shl_info.summary == "shift left" &&
           shr_info.name == "shr" && shr_info.signature == "shr(x, n)" &&
           shr_info.summary == "shift right" &&
           sqrt_info.name == "sqrt" && sqrt_info.min_arity == 1 && sqrt_info.max_arity == 1 &&
           sqrt_info.category == console_calc::BuiltinFunctionCategory::scalar &&
           sqrt_info.summary == "square root" && sqrt_info.mappable &&
           rand_info.name == "rand" && rand_info.min_arity == 0 && rand_info.max_arity == 2 &&
           rand_info.category == console_calc::BuiltinFunctionCategory::scalar &&
           rand_info.signature == "rand([min, max])" &&
           rand_info.summary == "random number in half-open interval" &&
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
           first_info.name == "first" && first_info.signature == "first(list, n)" &&
           first_info.summary == "first n list elements" &&
           drop_info.name == "drop" && drop_info.signature == "drop(list, n)" &&
           drop_info.summary == "drop first n list elements" &&
           guard_info.name == "guard" && guard_info.min_arity == 2 &&
           guard_info.max_arity == 2 &&
           guard_info.category == console_calc::BuiltinFunctionCategory::scalar &&
           guard_info.signature == "guard(expr, fallback)" &&
           guard_info.summary == "use fallback when expr evaluation fails" &&
           pos_info.name == "pos" && pos_info.min_arity == 2 && pos_info.max_arity == 2 &&
           pos_info.category == console_calc::BuiltinFunctionCategory::position &&
           pos_info.signature == "pos(lat, lon)" && pos_info.scalar_arguments &&
           lat_info.name == "lat" && lat_info.min_arity == 1 && lat_info.max_arity == 1 &&
           lat_info.category == console_calc::BuiltinFunctionCategory::position &&
           lat_info.signature == "lat(pos)" && !lat_info.scalar_arguments &&
           lon_info.name == "lon" &&
           lon_info.category == console_calc::BuiltinFunctionCategory::position &&
           lon_info.signature == "lon(pos)" &&
           !lon_info.scalar_arguments &&
           to_list_info.name == "to_list" &&
           to_list_info.category == console_calc::BuiltinFunctionCategory::position &&
           to_list_info.signature == "to_list(poslist)" &&
           !to_list_info.scalar_arguments &&
           to_poslist_info.name == "to_poslist" &&
           to_poslist_info.category == console_calc::BuiltinFunctionCategory::position &&
           to_poslist_info.signature == "to_poslist(list)" &&
           !to_poslist_info.scalar_arguments &&
           densify_path_info.name == "densify_path" &&
           densify_path_info.min_arity == 2 &&
           densify_path_info.max_arity == 2 &&
           densify_path_info.category == console_calc::BuiltinFunctionCategory::position &&
           densify_path_info.signature == "densify_path(poslist, count)" &&
           !densify_path_info.scalar_arguments &&
           offset_path_info.name == "offset_path" &&
           offset_path_info.min_arity == 3 &&
           offset_path_info.max_arity == 3 &&
           offset_path_info.category == console_calc::BuiltinFunctionCategory::position &&
           offset_path_info.signature == "offset_path(poslist, offset_x_m, offset_y_m)" &&
           !offset_path_info.scalar_arguments &&
           simplify_path_info.name == "simplify_path" &&
           simplify_path_info.min_arity == 2 &&
           simplify_path_info.max_arity == 2 &&
           simplify_path_info.category == console_calc::BuiltinFunctionCategory::position &&
           simplify_path_info.signature == "simplify_path(poslist, tolerance_m)" &&
           !simplify_path_info.scalar_arguments &&
           compress_path_info.name == "compress_path" &&
           compress_path_info.min_arity == 2 &&
           compress_path_info.max_arity == 3 &&
           compress_path_info.category == console_calc::BuiltinFunctionCategory::position &&
           compress_path_info.signature == "compress_path(poslist, count[, max_points])" &&
           !compress_path_info.scalar_arguments &&
           dist_info.name == "dist" &&
           dist_info.min_arity == 1 &&
           dist_info.max_arity == 2 &&
           dist_info.category == console_calc::BuiltinFunctionCategory::position &&
           dist_info.signature == "dist(pos1, pos2) / dist(poslist)" &&
           !dist_info.scalar_arguments &&
           bearing_info.name == "bearing" &&
           bearing_info.category == console_calc::BuiltinFunctionCategory::position &&
           bearing_info.signature == "bearing(pos1, pos2)" &&
           !bearing_info.scalar_arguments &&
           br_to_pos_info.name == "br_to_pos" &&
           br_to_pos_info.category == console_calc::BuiltinFunctionCategory::position &&
           br_to_pos_info.signature == "br_to_pos(pos, bearing_deg, range_m)" &&
           !br_to_pos_info.scalar_arguments &&
           reduce_info.name == "reduce" && reduce_info.min_arity == 2 &&
           reduce_info.max_arity == 2 &&
           reduce_info.category == console_calc::BuiltinFunctionCategory::list &&
           reduce_info.summary == "reduce list with binary operator" &&
           timed_loop_info.name == "timed_loop" &&
           timed_loop_info.min_arity == 2 && timed_loop_info.max_arity == 2 &&
           timed_loop_info.category == console_calc::BuiltinFunctionCategory::scalar &&
           timed_loop_info.signature == "timed_loop(expr, count)" &&
           timed_loop_info.summary ==
               "evaluate expr count times and return elapsed seconds" &&
           fill_info.name == "fill" && fill_info.min_arity == 2 && fill_info.max_arity == 2 &&
           fill_info.category == console_calc::BuiltinFunctionCategory::list_generation &&
           fill_info.signature == "fill(expr, count)" &&
           fill_info.summary == "evaluate expr count times into a list" &&
           map_info.name == "map" && map_info.min_arity == 2 && map_info.max_arity == 5 &&
           map_info.category == console_calc::BuiltinFunctionCategory::list &&
           map_info.signature == "map(list, expr[, start[, step[, count]]])" &&
           map_info.summary == "map inline expression over list slice" &&
           map_at_info.name == "map_at" && map_at_info.min_arity == 2 &&
           map_at_info.max_arity == 5 &&
           map_at_info.category == console_calc::BuiltinFunctionCategory::list &&
           map_at_info.signature == "map_at(list, expr[, start[, step[, count]]])" &&
           map_at_info.summary == "map inline expression onto selected list positions" &&
           list_where_info.name == "list_where" && list_where_info.min_arity == 2 &&
           list_where_info.max_arity == 2 &&
           list_where_info.category == console_calc::BuiltinFunctionCategory::list &&
           list_where_info.signature == "list_where(list, expr)" &&
           list_where_info.summary == "keep list elements where inline expression is non-zero" &&
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
           console_calc::is_scalar_function(console_calc::Function::timed_loop) &&
           console_calc::is_scalar_function(console_calc::Function::rand) &&
           !console_calc::is_scalar_function(console_calc::Function::pos) &&
           !console_calc::is_scalar_function(console_calc::Function::sum) &&
           console_calc::is_list_function(console_calc::Function::sum) &&
           console_calc::is_list_function(console_calc::Function::list_add) &&
           console_calc::is_list_function(console_calc::Function::fill) &&
           console_calc::is_list_function(console_calc::Function::range) &&
           !console_calc::is_list_function(console_calc::Function::pos) &&
           !console_calc::is_list_function(console_calc::Function::pow) &&
           console_calc::is_unary_scalar_function(console_calc::Function::sin) &&
           console_calc::is_unary_scalar_function(console_calc::Function::abs) &&
           console_calc::is_unary_scalar_function(console_calc::Function::sqrt) &&
           !console_calc::is_unary_scalar_function(console_calc::Function::rand) &&
           !console_calc::is_unary_scalar_function(console_calc::Function::lat) &&
           !console_calc::is_unary_scalar_function(console_calc::Function::pow) &&
           console_calc::is_mappable_unary_scalar_function(console_calc::Function::sin) &&
           console_calc::is_mappable_unary_scalar_function(console_calc::Function::abs) &&
           console_calc::is_mappable_unary_scalar_function(console_calc::Function::sqrt) &&
           !console_calc::is_mappable_unary_scalar_function(console_calc::Function::rand) &&
           !console_calc::is_mappable_unary_scalar_function(console_calc::Function::pow) &&
           !console_calc::is_mappable_unary_scalar_function(console_calc::Function::sum);
}

bool expect_expression_identifier_expansion() {
    const console_calc::ConstantTable constants = console_calc::builtin_constant_table();
    const console_calc::DefinitionTable definitions{
        {"x", console_calc::make_value_definition("pi + 1")},
        {"vals", console_calc::make_value_definition("{1, 2, 3}")},
        {"f", console_calc::make_function_definition({"arg"}, "arg + 1")},
        {"pair_sum", console_calc::make_function_definition({"a", "b"}, "a + b")},
        {"p", console_calc::make_function_definition({"x"}, "x % 2 = 1")},
    };

    return expect_equal("expand sin(x)",
                        console_calc::expand_expression_identifiers(
                            "sin(x)", constants, definitions, std::nullopt),
                        "sin((3.1415926535897931 + 1))") &&
           expect_equal("expand sum(vals)",
                        console_calc::expand_expression_identifiers(
                            "sum(vals)", constants, definitions, std::nullopt),
                        "sum({1, 2, 3})") &&
           expect_equal("expand map(vals, sin(_) + _)",
                        console_calc::expand_expression_identifiers(
                            "map(vals, sin(_) + _)", constants, definitions, std::nullopt),
                        "map({1, 2, 3}, sin(_) + _)") &&
           expect_equal("expand map_at(vals, sin(_) + _)",
                        console_calc::expand_expression_identifiers(
                            "map_at(vals, sin(_) + _)", constants, definitions, std::nullopt),
                        "map_at({1, 2, 3}, sin(_) + _)") &&
           expect_equal("expand list_where(vals, _ <= 2)",
                        console_calc::expand_expression_identifiers(
                            "list_where(vals, _ <= 2)", constants, definitions, std::nullopt),
                        "list_where({1, 2, 3}, _ <= 2)") &&
           expect_equal("expand f(3)",
                        console_calc::expand_expression_identifiers(
                            "f(3)", constants, definitions, std::nullopt),
                        "((3) + 1)") &&
           expect_equal("expand f(f(3))",
                        console_calc::expand_expression_identifiers(
                            "f(f(3))", constants, definitions, std::nullopt),
                        "((((3) + 1)) + 1)") &&
           expect_equal("expand pair_sum(2, 5)",
                        console_calc::expand_expression_identifiers(
                            "pair_sum(2, 5)", constants, definitions, std::nullopt),
                        "((2) + (5))") &&
           expect_equal("expand map(vals, f(_))",
                        console_calc::expand_expression_identifiers(
                            "map(vals, f(_))", constants, definitions, std::nullopt),
                        "map({1, 2, 3}, ((_) + 1))") &&
           expect_equal("expand list_where(vals, p(_))",
                        console_calc::expand_expression_identifiers(
                            "list_where(vals, p(_))", constants, definitions, std::nullopt),
                        "list_where({1, 2, 3}, ((_) % 2 = 1))") &&
           expect_equal("expand 0x10 + 5",
                        console_calc::expand_expression_identifiers(
                            "0x10 + 5", constants, definitions, std::nullopt),
                        "0x10 + 5") &&
           expect_equal("expand 0b1010 + 1",
                        console_calc::expand_expression_identifiers(
                            "0b1010 + 1", constants, definitions, std::nullopt),
                        "0b1010 + 1") &&
           expect_equal("expand 90*c.deg",
                        console_calc::expand_expression_identifiers(
                            "90*c.deg", constants, definitions, std::nullopt),
                        "90*0.017453292519943295") &&
           expect_equal("expand m.pi",
                        console_calc::expand_expression_identifiers(
                            "m.pi", constants, definitions, std::nullopt),
                        "3.1415926535897931") &&
           expect_equal("expand ph.c",
                        console_calc::expand_expression_identifiers(
                            "ph.c", constants, definitions, std::nullopt),
                        "299792458");
}

bool expect_user_assignment_parsing() {
    const auto value_assignment = console_calc::parse_user_assignment("x:pi+1");
    const auto echoed_value_assignment = console_calc::parse_user_assignment("#x:pi+1");
    const auto function_assignment = console_calc::parse_user_assignment("f(x):x+1");
    const auto multi_function_assignment = console_calc::parse_user_assignment("f(x,y):x+y");
    const auto invalid_assignment = console_calc::parse_user_assignment("f(x,x):x+y");

    return value_assignment.has_value() && value_assignment->name == "x" &&
           value_assignment->parameters.empty() &&
           value_assignment->expression == "pi+1" && !value_assignment->emit_result &&
           echoed_value_assignment.has_value() &&
           echoed_value_assignment->name == "x" &&
           echoed_value_assignment->parameters.empty() &&
           echoed_value_assignment->expression == "pi+1" &&
           echoed_value_assignment->emit_result &&
           function_assignment.has_value() &&
           function_assignment->name == "f" && function_assignment->parameters.size() == 1 &&
           function_assignment->parameters[0] == "x" &&
           function_assignment->expression == "x+1" && !function_assignment->emit_result &&
           multi_function_assignment.has_value() &&
           multi_function_assignment->name == "f" &&
           multi_function_assignment->parameters ==
               std::vector<std::string>({"x", "y"}) &&
           multi_function_assignment->expression == "x+y" &&
           !multi_function_assignment->emit_result && !invalid_assignment.has_value();
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

    if (!expect_user_assignment_parsing()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
