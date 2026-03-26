#include "console_calc/builtin_function.h"

#include <array>
#include <string>
#include <stdexcept>

#include "console_calc/special_form.h"

namespace console_calc {

namespace {

constexpr std::array<BuiltinFunctionInfo, 48> k_builtin_functions = {{
    {Function::abs, "abs", 1, 1, BuiltinFunctionCategory::scalar, true, true, "abs(x)", "absolute value"},
    {Function::sin, "sin", 1, 1, BuiltinFunctionCategory::scalar, true, true, "sin(x)", "sine in radians"},
    {Function::cos, "cos", 1, 1, BuiltinFunctionCategory::scalar, true, true, "cos(x)", "cosine in radians"},
    {Function::tan, "tan", 1, 1, BuiltinFunctionCategory::scalar, true, true, "tan(x)", "tangent in radians"},
    {Function::sind, "sind", 1, 1, BuiltinFunctionCategory::scalar, true, true, "sind(x)", "sine in degrees"},
    {Function::cosd, "cosd", 1, 1, BuiltinFunctionCategory::scalar, true, true, "cosd(x)", "cosine in degrees"},
    {Function::tand, "tand", 1, 1, BuiltinFunctionCategory::scalar, true, true, "tand(x)", "tangent in degrees"},
    {Function::sqrt, "sqrt", 1, 1, BuiltinFunctionCategory::scalar, true, true, "sqrt(x)", "square root"},
    {Function::bit_and, "and", 2, 2, BuiltinFunctionCategory::scalar, true, false, "and(a, b)", "bitwise and"},
    {Function::bit_or, "or", 2, 2, BuiltinFunctionCategory::scalar, true, false, "or(a, b)", "bitwise or"},
    {Function::bit_xor, "xor", 2, 2, BuiltinFunctionCategory::scalar, true, false, "xor(a, b)", "bitwise exclusive or"},
    {Function::bit_nand, "nand", 2, 2, BuiltinFunctionCategory::scalar, true, false, "nand(a, b)", "bitwise not-and"},
    {Function::bit_nor, "nor", 2, 2, BuiltinFunctionCategory::scalar, true, false, "nor(a, b)", "bitwise not-or"},
    {Function::shl, "shl", 2, 2, BuiltinFunctionCategory::scalar, true, false, "shl(x, n)", "shift left"},
    {Function::shr, "shr", 2, 2, BuiltinFunctionCategory::scalar, true, false, "shr(x, n)", "shift right"},
    {Function::pow, "pow", 2, 2, BuiltinFunctionCategory::scalar, true, false, "pow(x, y)", "power"},
    {Function::rand, "rand", 0, 2, BuiltinFunctionCategory::scalar, true, false, "rand([min, max])", "random number in half-open interval"},
    {Function::pos, "pos", 2, 2, BuiltinFunctionCategory::position, true, false, "pos(lat, lon)", "construct WGS84 position in degrees"},
    {Function::lat, "lat", 1, 1, BuiltinFunctionCategory::position, false, false, "lat(pos)", "extract latitude in degrees"},
    {Function::lon, "lon", 1, 1, BuiltinFunctionCategory::position, false, false, "lon(pos)", "extract longitude in degrees"},
    {Function::to_poslist, "to_poslist", 1, 1, BuiltinFunctionCategory::position, false, false, "to_poslist(list)", "pair scalar list values into positions"},
    {Function::to_list, "to_list", 1, 1, BuiltinFunctionCategory::position, false, false, "to_list(poslist|multi_pos_list)", "expand positions into lat lon scalar list(s)"},
    {Function::densify_path, "densify_path", 2, 2, BuiltinFunctionCategory::position, false, false, "densify_path(poslist|multi_pos_list, count)", "insert evenly spaced geodesic points per leg"},
    {Function::offset_path, "offset_path", 3, 3, BuiltinFunctionCategory::position, false, false, "offset_path(poslist|multi_pos_list, offset_x_m, offset_y_m)", "translate path list(s) in a midpoint-centered local right/forward frame; repeated application is only approximately reversible"},
    {Function::rotate_path, "rotate_path", 2, 3, BuiltinFunctionCategory::position, false, false, "rotate_path(poslist|multi_pos_list, degrees[, center_index])", "rotate path list(s) around a weighted center or chosen point index using center-relative geodesic bearings"},
    {Function::scale_path, "scale_path", 2, 2, BuiltinFunctionCategory::position, false, false, "scale_path(poslist|multi_pos_list, scale_factor)", "scale path list(s) around poslist[N/2] in azimuthal-equidistant coordinates"},
    {Function::simplify_path, "simplify_path", 2, 2, BuiltinFunctionCategory::position, false, false, "simplify_path(poslist|multi_pos_list, tolerance_m)", "remove path points within tolerance"},
    {Function::compress_path, "compress_path", 2, 3, BuiltinFunctionCategory::position, false, false, "compress_path(poslist|multi_pos_list, count[, max_points])", "remove path points to reach an exact count"},
    {Function::dist, "dist", 1, 2, BuiltinFunctionCategory::position, false, false, "dist(pos1, pos2) / dist(poslist|multi_pos_list)", "WGS84 ellipsoid distance, path length, or per-path lengths in meters"},
    {Function::bearing, "bearing", 2, 2, BuiltinFunctionCategory::position, false, false, "bearing(pos1, pos2)", "initial WGS84 bearing in degrees"},
    {Function::br_to_pos, "br_to_pos", 3, 3, BuiltinFunctionCategory::position, false, false, "br_to_pos(pos, bearing_deg, range_m)", "destination position from bearing and range"},
    {Function::sum, "sum", 1, 1, BuiltinFunctionCategory::list, true, false, "sum(list|multilist)", "sum list elements or each inner list"},
    {Function::len, "len", 1, 1, BuiltinFunctionCategory::list, true, false, "len(list|multilist|poslist|multi_pos_list)", "collection length"},
    {Function::product, "product", 1, 1, BuiltinFunctionCategory::list, true, false, "product(list|multilist)", "product of list elements or each inner list"},
    {Function::avg, "avg", 1, 1, BuiltinFunctionCategory::list, true, false, "avg(list|multilist)", "average of list elements or each inner list"},
    {Function::min, "min", 1, 1, BuiltinFunctionCategory::list, true, false, "min(list|multilist)", "minimum list element or each inner list"},
    {Function::max, "max", 1, 1, BuiltinFunctionCategory::list, true, false, "max(list|multilist)", "maximum list element or each inner list"},
    {Function::first, "first", 2, 2, BuiltinFunctionCategory::list, true, false, "first(list|multilist, n)", "first n list elements or each inner list"},
    {Function::drop, "drop", 2, 2, BuiltinFunctionCategory::list, true, false, "drop(list|multilist, n)", "drop first n list elements or each inner list"},
    {Function::list_add, "list_add", 2, 2, BuiltinFunctionCategory::list, true, false,
     "list_add(a, b)", "add matching list elements"},
    {Function::list_sub, "list_sub", 2, 2, BuiltinFunctionCategory::list, true, false,
     "list_sub(a, b)", "subtract matching list elements"},
    {Function::list_div, "list_div", 2, 2, BuiltinFunctionCategory::list, true, false,
     "list_div(a, b)", "divide matching list elements"},
    {Function::list_mul, "list_mul", 2, 2, BuiltinFunctionCategory::list, true, false,
     "list_mul(a, b)", "multiply matching list elements"},
    {Function::range, "range", 2, 3, BuiltinFunctionCategory::list_generation, true, false,
     "range(start, count[, step])", "generate linear series from start"},
    {Function::geom, "geom", 2, 3, BuiltinFunctionCategory::list_generation, true, false,
     "geom(start, count[, ratio])", "generate geometric series from start"},
    {Function::repeat, "repeat", 2, 2, BuiltinFunctionCategory::list_generation, true, false,
     "repeat(value, count)", "repeat value count times"},
    {Function::linspace, "linspace", 3, 3, BuiltinFunctionCategory::list_generation, true, false,
     "linspace(start, stop, count)", "generate evenly spaced values over interval"},
    {Function::powers, "powers", 2, 3, BuiltinFunctionCategory::list_generation, true, false,
     "powers(base, count[, start_exp])", "generate successive integer powers"},
}};

}  // namespace

std::optional<Function> parse_builtin_function(std::string_view name) {
    for (const auto& function : k_builtin_functions) {
        if (function.name == name) {
            return function.function;
        }
    }

    return std::nullopt;
}

const BuiltinFunctionInfo& builtin_function_info(Function function) {
    for (const auto& info : k_builtin_functions) {
        if (info.function == function) {
            return info;
        }
    }

    throw std::invalid_argument("unknown builtin function");
}

std::string_view builtin_function_name(Function function) {
    if (is_special_form(function)) {
        return special_form_info(function).name;
    }
    return builtin_function_info(function).name;
}

bool builtin_function_accepts_arity(Function function, std::size_t arity) {
    if (is_special_form(function)) {
        return special_form_accepts_arity(function, arity);
    }
    const auto& info = builtin_function_info(function);
    return arity >= info.min_arity && arity <= info.max_arity;
}

std::string builtin_function_arity_label(Function function) {
    if (is_special_form(function)) {
        const auto& info = special_form_info(function);
        if (info.min_arity == info.max_arity) {
            return std::to_string(info.min_arity);
        }
        return std::to_string(info.min_arity) + "-" + std::to_string(info.max_arity);
    }
    const auto& info = builtin_function_info(function);
    if (info.min_arity == info.max_arity) {
        return std::to_string(info.min_arity);
    }
    return std::to_string(info.min_arity) + "-" + std::to_string(info.max_arity);
}

std::string_view builtin_function_signature(Function function) {
    if (is_special_form(function)) {
        return special_form_signature(function);
    }
    return builtin_function_info(function).signature;
}

bool is_builtin_function_name(std::string_view name) {
    return parse_builtin_function(name).has_value();
}

std::span<const BuiltinFunctionInfo> builtin_functions() {
    return k_builtin_functions;
}

bool is_scalar_function(Function function) {
    if (is_special_form(function)) {
        return special_form_info(function).category == BuiltinFunctionCategory::scalar;
    }
    return builtin_function_info(function).category == BuiltinFunctionCategory::scalar;
}

bool is_list_function(Function function) {
    if (is_special_form(function)) {
        const auto category = special_form_info(function).category;
        return category == BuiltinFunctionCategory::list ||
               category == BuiltinFunctionCategory::list_generation;
    }
    const auto category = builtin_function_info(function).category;
    return category == BuiltinFunctionCategory::list ||
           category == BuiltinFunctionCategory::list_generation;
}

bool is_unary_scalar_function(Function function) {
    if (is_special_form(function)) {
        return false;
    }
    const auto& info = builtin_function_info(function);
    return info.category == BuiltinFunctionCategory::scalar && info.scalar_arguments &&
           info.min_arity == 1 &&
           info.max_arity == 1;
}

bool is_mappable_unary_scalar_function(Function function) {
    if (is_special_form(function)) {
        return false;
    }
    const auto& info = builtin_function_info(function);
    return info.category == BuiltinFunctionCategory::scalar && info.scalar_arguments &&
           info.min_arity == 1 &&
           info.max_arity == 1 && info.mappable;
}

}  // namespace console_calc
