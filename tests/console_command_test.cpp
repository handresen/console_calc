#include <cstdlib>
#include <string>

#include "console_command.h"
#include "console_listing.h"
#include "expression_environment.h"
#include "console_calc/builtin_function.h"
#include "console_calc/special_form.h"

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
    const auto sqrt_info = console_calc::builtin_function_info(console_calc::Function::sqrt);
    const auto rand_info = console_calc::builtin_function_info(console_calc::Function::rand);
    const auto list_add_info = console_calc::builtin_function_info(console_calc::Function::list_add);
    const auto list_div_info = console_calc::builtin_function_info(console_calc::Function::list_div);
    const auto list_mul_info = console_calc::builtin_function_info(console_calc::Function::list_mul);
    const auto list_sub_info = console_calc::builtin_function_info(console_calc::Function::list_sub);
    const auto guard_info = console_calc::special_form_info(console_calc::Function::guard);
    const auto pos_info = console_calc::builtin_function_info(console_calc::Function::pos);
    const auto lat_info = console_calc::builtin_function_info(console_calc::Function::lat);
    const auto lon_info = console_calc::builtin_function_info(console_calc::Function::lon);
    const auto to_list_info = console_calc::builtin_function_info(console_calc::Function::to_list);
    const auto to_poslist_info =
        console_calc::builtin_function_info(console_calc::Function::to_poslist);
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
           dist_info.name == "dist" &&
           dist_info.category == console_calc::BuiltinFunctionCategory::position &&
           dist_info.signature == "dist(pos1, pos2)" &&
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
               "map_at(vals, sin(_) + _)", constants, definitions, std::nullopt) ==
               "map_at({1, 2, 3}, sin(_) + _)" &&
           console_calc::expand_expression_identifiers(
               "list_where(vals, _ <= 2)", constants, definitions, std::nullopt) ==
               "list_where({1, 2, 3}, _ <= 2)" &&
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
