#include "expression_evaluator_internal.h"

#include "console_calc/expression_error.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <random>
#include <span>
#include <string_view>

#include "geodesy.h"
#include "scalar_math.h"

namespace console_calc::detail {

namespace {

[[nodiscard]] double degrees_to_radians(double value) {
    return value * std::numbers::pi_v<double> / 180.0;
}

[[nodiscard]] double evaluate_unary_scalar_builtin(Function function, double argument) {
    switch (function) {
    case Function::abs:
        return require_finite_result(std::fabs(argument));
    case Function::sin:
        return require_finite_result(std::sin(argument));
    case Function::cos:
        return require_finite_result(std::cos(argument));
    case Function::tan:
        return require_finite_result(std::tan(argument));
    case Function::sind:
        return require_finite_result(std::sin(degrees_to_radians(argument)));
    case Function::cosd:
        return require_finite_result(std::cos(degrees_to_radians(argument)));
    case Function::tand:
        return require_finite_result(std::tan(degrees_to_radians(argument)));
    case Function::sqrt:
        if (argument < 0.0) {
            throw EvaluationError("sqrt() requires a non-negative input");
        }
        return require_finite_result(std::sqrt(argument));
    default:
        break;
    }

    throw EvaluationError("function cannot be applied elementwise");
}

template <typename Operation>
[[nodiscard]] Value evaluate_pairwise_list_builtin(std::span<const Value> arguments,
                                                   std::string_view function_name,
                                                   Operation operation) {
    const ListValue lhs = require_list(arguments[0]);
    const ListValue rhs = require_list(arguments[1]);
    if (lhs.size() != rhs.size()) {
        throw EvaluationError(std::string(function_name) + "() requires lists of equal length");
    }

    ListValue values;
    values.reserve(lhs.size());
    for (std::size_t index = 0; index < lhs.size(); ++index) {
        values.push_back(operation(lhs[index], rhs[index]));
    }
    return values;
}

[[nodiscard]] Value evaluate_scalar_builtin(Function function, std::span<const Value> arguments) {
    switch (function) {
    case Function::abs: {
        const ScalarValue value = require_scalar_or_singleton_list_value(arguments[0]);
        if (const auto* integer = std::get_if<std::int64_t>(&value)) {
            if (*integer == std::numeric_limits<std::int64_t>::min()) {
                return require_finite_result(std::fabs(static_cast<double>(*integer)));
            }
            return *integer < 0 ? to_value(-*integer) : to_value(*integer);
        }
        return require_finite_result(std::fabs(std::get<double>(value)));
    }
    case Function::sin:
    case Function::cos:
    case Function::tan:
    case Function::sind:
    case Function::cosd:
    case Function::tand:
    case Function::sqrt:
        return evaluate_unary_scalar_builtin(
            function, scalar_to_double(require_scalar_or_singleton_list_value(arguments[0])));
    case Function::bit_and:
        return to_value(static_cast<std::int64_t>(
            require_integer_operand(require_scalar_or_singleton_list_value(arguments[0])) &
            require_integer_operand(require_scalar_or_singleton_list_value(arguments[1]))));
    case Function::bit_or:
        return to_value(static_cast<std::int64_t>(
            require_integer_operand(require_scalar_or_singleton_list_value(arguments[0])) |
            require_integer_operand(require_scalar_or_singleton_list_value(arguments[1]))));
    case Function::bit_xor:
        return to_value(static_cast<std::int64_t>(
            require_integer_operand(require_scalar_or_singleton_list_value(arguments[0])) ^
            require_integer_operand(require_scalar_or_singleton_list_value(arguments[1]))));
    case Function::bit_nand:
        return to_value(static_cast<std::int64_t>(~(
            require_integer_operand(require_scalar_or_singleton_list_value(arguments[0])) &
            require_integer_operand(require_scalar_or_singleton_list_value(arguments[1])))));
    case Function::bit_nor:
        return to_value(static_cast<std::int64_t>(~(
            require_integer_operand(require_scalar_or_singleton_list_value(arguments[0])) |
            require_integer_operand(require_scalar_or_singleton_list_value(arguments[1])))));
    case Function::shl: {
        const auto value =
            require_integer_operand(require_scalar_or_singleton_list_value(arguments[0]));
        const auto shift =
            require_integer_operand(require_scalar_or_singleton_list_value(arguments[1]));
        if (shift < 0 || shift >= 64) {
            throw EvaluationError("shl() shift must be in range 0..63");
        }
        return to_value(static_cast<std::int64_t>(value << shift));
    }
    case Function::shr: {
        const auto value =
            require_integer_operand(require_scalar_or_singleton_list_value(arguments[0]));
        const auto shift =
            require_integer_operand(require_scalar_or_singleton_list_value(arguments[1]));
        if (shift < 0 || shift >= 64) {
            throw EvaluationError("shr() shift must be in range 0..63");
        }
        return to_value(static_cast<std::int64_t>(value >> shift));
    }
    case Function::pow:
        return to_value(power_scalars(
            require_scalar_or_singleton_list_value(arguments[0]),
            require_scalar_or_singleton_list_value(arguments[1])));
    case Function::rand: {
        static thread_local std::mt19937_64 generator(
            static_cast<std::mt19937_64::result_type>(
                std::chrono::steady_clock::now().time_since_epoch().count()) ^
            (static_cast<std::mt19937_64::result_type>(std::random_device{}()) << 1U));

        double minimum = 0.0;
        double maximum = 1.0;
        if (arguments.size() == 1) {
            maximum = scalar_to_double(require_scalar_or_singleton_list_value(arguments[0]));
        } else if (arguments.size() == 2) {
            minimum = scalar_to_double(require_scalar_or_singleton_list_value(arguments[0]));
            maximum = scalar_to_double(require_scalar_or_singleton_list_value(arguments[1]));
        }

        if (!std::isfinite(minimum) || !std::isfinite(maximum)) {
            throw EvaluationError("rand() bounds must be finite");
        }
        if (!(minimum < maximum)) {
            throw EvaluationError("rand() requires min < max");
        }

        return std::uniform_real_distribution<double>(minimum, maximum)(generator);
    }
    default:
        break;
    }

    throw EvaluationError("unknown scalar builtin");
}

[[nodiscard]] Value evaluate_position_builtin(Function function, std::span<const Value> arguments) {
    switch (function) {
    case Function::pos:
        return normalize_position(
            scalar_to_double(require_scalar_or_singleton_list_value(arguments[0])),
            scalar_to_double(require_scalar_or_singleton_list_value(arguments[1])));
    case Function::lat:
        return require_position(arguments[0]).latitude_deg;
    case Function::lon:
        return require_position(arguments[0]).longitude_deg;
    case Function::to_poslist: {
        const ListValue values = require_list(arguments[0]);
        if (values.empty()) {
            return PositionListValue{};
        }
        if ((values.size() % 2U) != 0U) {
            throw EvaluationError("to_poslist() requires an even number of scalar values");
        }

        PositionListValue positions;
        positions.reserve(values.size() / 2U);
        for (std::size_t index = 0; index < values.size(); index += 2U) {
            positions.push_back(normalize_position(scalar_to_double(values[index]),
                                                   scalar_to_double(values[index + 1U])));
        }
        return positions;
    }
    case Function::to_list: {
        const PositionListValue positions = require_position_list(arguments[0]);
        ListValue values;
        values.reserve(positions.size() * 2U);
        for (const auto& position : positions) {
            values.push_back(position.latitude_deg);
            values.push_back(position.longitude_deg);
        }
        return values;
    }
    case Function::densify_path: {
        const PositionListValue positions = require_position_list(arguments[0]);
        const auto inserted_per_leg =
            require_integer_operand(require_scalar_or_singleton_list_value(arguments[1]));
        if (inserted_per_leg < 0) {
            throw EvaluationError("densify_path() count must be a non-negative integer");
        }
        return densify_wgs84_path(positions, static_cast<std::size_t>(inserted_per_leg));
    }
    case Function::offset_path:
        return offset_wgs84_path(
            require_position_list(arguments[0]),
            scalar_to_double(require_scalar_or_singleton_list_value(arguments[1])),
            scalar_to_double(require_scalar_or_singleton_list_value(arguments[2])));
    case Function::rotate_path: {
        const PositionListValue positions = require_position_list(arguments[0]);
        const auto center_index =
            require_integer_operand(require_scalar_or_singleton_list_value(arguments[1]));
        if (center_index < 0) {
            throw EvaluationError("rotate_path() center index must be non-negative");
        }
        return rotate_wgs84_path(
            positions, static_cast<std::size_t>(center_index),
            scalar_to_double(require_scalar_or_singleton_list_value(arguments[2])));
    }
    case Function::scale_path:
        return scale_wgs84_path(
            require_position_list(arguments[0]),
            scalar_to_double(require_scalar_or_singleton_list_value(arguments[1])));
    case Function::simplify_path:
        return simplify_wgs84_path(
            require_position_list(arguments[0]),
            scalar_to_double(require_scalar_or_singleton_list_value(arguments[1])));
    case Function::compress_path: {
        constexpr std::int64_t k_default_max_points = 5000;
        const PositionListValue positions = require_position_list(arguments[0]);
        const auto target_count =
            require_integer_operand(require_scalar_or_singleton_list_value(arguments[1]));
        const auto max_points =
            arguments.size() == 3U
                ? require_integer_operand(require_scalar_or_singleton_list_value(arguments[2]))
                : k_default_max_points;
        if (target_count < 0) {
            throw EvaluationError("compress_path() target count must be non-negative");
        }
        if (max_points < 0) {
            throw EvaluationError("compress_path() max_points must be non-negative");
        }
        return compress_wgs84_path(positions, static_cast<std::size_t>(target_count),
                                   static_cast<std::size_t>(max_points));
    }
    case Function::dist:
        if (arguments.size() == 1U) {
            return wgs84_path_distance(require_position_list(arguments[0]));
        }
        return wgs84_inverse(require_position(arguments[0]), require_position(arguments[1]))
            .distance_m;
    case Function::bearing:
        return wgs84_inverse(require_position(arguments[0]), require_position(arguments[1]))
            .initial_bearing_deg;
    case Function::br_to_pos:
        return wgs84_direct(
            require_position(arguments[0]),
            scalar_to_double(require_scalar_or_singleton_list_value(arguments[1])),
            scalar_to_double(require_scalar_or_singleton_list_value(arguments[2])));
    default:
        break;
    }

    throw EvaluationError("unknown position builtin");
}

[[nodiscard]] Value evaluate_list_builtin(Function function, std::span<const Value> arguments) {
    switch (function) {
    case Function::sum: {
        const ListValue values = require_list(arguments[0]);
        ScalarValue total = std::int64_t{0};
        for (const auto& value : values) {
            total = add_scalars(total, value);
        }
        return to_value(total);
    }
    case Function::len: {
        if (const auto* values = std::get_if<ListValue>(&arguments[0])) {
            return static_cast<std::int64_t>(values->size());
        }
        if (const auto* values = std::get_if<MultiListValue>(&arguments[0])) {
            return static_cast<std::int64_t>(values->size());
        }
        if (const auto* positions = std::get_if<PositionListValue>(&arguments[0])) {
            return static_cast<std::int64_t>(positions->size());
        }
        if (const auto* positions = std::get_if<MultiPositionListValue>(&arguments[0])) {
            return static_cast<std::int64_t>(positions->size());
        }
        throw EvaluationError("list, multi-list, position list, or multi position list value required");
    }
    case Function::product: {
        const ListValue values = require_list(arguments[0]);
        ScalarValue total = std::int64_t{1};
        for (const auto& value : values) {
            total = multiply_scalars(total, value);
        }
        return to_value(total);
    }
    case Function::avg: {
        const ListValue values = require_list(arguments[0]);
        if (values.empty()) {
            throw EvaluationError("avg() requires a non-empty list");
        }

        double total = 0.0;
        for (const auto& value : values) {
            total = require_finite_result(total + scalar_to_double(value));
        }
        return require_finite_result(total / static_cast<double>(values.size()));
    }
    case Function::min: {
        const ListValue values = require_list(arguments[0]);
        if (values.empty()) {
            throw EvaluationError("min() requires a non-empty list");
        }

        ScalarValue result = values.front();
        for (std::size_t index = 1; index < values.size(); ++index) {
            if (scalar_to_double(values[index]) < scalar_to_double(result)) {
                result = values[index];
            }
        }
        return to_value(result);
    }
    case Function::max: {
        const ListValue values = require_list(arguments[0]);
        if (values.empty()) {
            throw EvaluationError("max() requires a non-empty list");
        }

        ScalarValue result = values.front();
        for (std::size_t index = 1; index < values.size(); ++index) {
            if (scalar_to_double(values[index]) > scalar_to_double(result)) {
                result = values[index];
            }
        }
        return to_value(result);
    }
    case Function::first: {
        const std::size_t count =
            require_list_index(require_scalar_or_singleton_list_value(arguments[1]));
        const ListValue values = require_list(arguments[0]);
        const std::size_t result_size = std::min(count, values.size());
        return ListValue(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(result_size));
    }
    case Function::drop: {
        const std::size_t count =
            require_list_index(require_scalar_or_singleton_list_value(arguments[1]));
        const ListValue values = require_list(arguments[0]);
        const std::size_t skip = std::min(count, values.size());
        return ListValue(values.begin() + static_cast<std::ptrdiff_t>(skip), values.end());
    }
    case Function::list_add:
        return evaluate_pairwise_list_builtin(arguments, "list_add", add_scalars);
    case Function::list_sub:
        return evaluate_pairwise_list_builtin(arguments, "list_sub", subtract_scalars);
    case Function::list_div:
        return evaluate_pairwise_list_builtin(arguments, "list_div", divide_scalars);
    case Function::list_mul:
        return evaluate_pairwise_list_builtin(arguments, "list_mul", multiply_scalars);
    default:
        break;
    }

    throw EvaluationError("unknown list builtin");
}

[[nodiscard]] Value evaluate_list_generation_builtin(Function function,
                                                     std::span<const Value> arguments) {
    switch (function) {
    case Function::range: {
        const ScalarValue start = require_scalar_or_singleton_list_value(arguments[0]);
        const std::size_t count =
            require_list_index(require_scalar_or_singleton_list_value(arguments[1]));
        const ScalarValue step =
            arguments.size() == 3 ? require_scalar_or_singleton_list_value(arguments[2])
                                  : ScalarValue{std::int64_t{1}};

        ListValue values;
        values.reserve(count);
        ScalarValue current = start;
        for (std::size_t index = 0; index < count; ++index) {
            values.push_back(current);
            current = add_scalars(current, step);
        }
        return values;
    }
    case Function::geom: {
        const ScalarValue start = require_scalar_or_singleton_list_value(arguments[0]);
        const std::size_t count =
            require_list_index(require_scalar_or_singleton_list_value(arguments[1]));
        const ScalarValue ratio =
            arguments.size() == 3 ? require_scalar_or_singleton_list_value(arguments[2])
                                  : ScalarValue{std::int64_t{2}};

        ListValue values;
        values.reserve(count);
        ScalarValue current = start;
        for (std::size_t index = 0; index < count; ++index) {
            values.push_back(current);
            current = multiply_scalars(current, ratio);
        }
        return values;
    }
    case Function::repeat: {
        const ScalarValue value = require_scalar_or_singleton_list_value(arguments[0]);
        const std::size_t count =
            require_list_index(require_scalar_or_singleton_list_value(arguments[1]));
        return ListValue(count, value);
    }
    case Function::linspace: {
        const ScalarValue start = require_scalar_or_singleton_list_value(arguments[0]);
        const ScalarValue stop = require_scalar_or_singleton_list_value(arguments[1]);
        const std::size_t count =
            require_list_index(require_scalar_or_singleton_list_value(arguments[2]));

        ListValue values;
        values.reserve(count);
        if (count == 0) {
            return values;
        }

        values.push_back(start);
        if (count == 1) {
            return values;
        }

        const ScalarValue denominator = static_cast<std::int64_t>(count - 1);
        const ScalarValue step = divide_scalars(subtract_scalars(stop, start), denominator);
        ScalarValue current = start;
        for (std::size_t index = 1; index < count; ++index) {
            current = add_scalars(current, step);
            values.push_back(current);
        }
        return values;
    }
    case Function::powers: {
        const ScalarValue base = require_scalar_or_singleton_list_value(arguments[0]);
        const std::size_t count =
            require_list_index(require_scalar_or_singleton_list_value(arguments[1]));
        const ScalarValue start_exponent =
            arguments.size() == 3 ? require_scalar_or_singleton_list_value(arguments[2])
                                  : ScalarValue{std::int64_t{0}};

        ListValue values;
        values.reserve(count);
        for (std::size_t index = 0; index < count; ++index) {
            const ScalarValue exponent =
                add_scalars(start_exponent, static_cast<std::int64_t>(index));
            values.push_back(power_scalars(base, exponent));
        }
        return values;
    }
    default:
        break;
    }

    throw EvaluationError("unknown list generation builtin");
}

}  // namespace

Value evaluate_builtin_function(Function function, std::span<const Value> arguments) {
    const auto category = builtin_function_info(function).category;

    if (category == BuiltinFunctionCategory::scalar) {
        return evaluate_scalar_builtin(function, arguments);
    }

    if (category == BuiltinFunctionCategory::position) {
        return evaluate_position_builtin(function, arguments);
    }

    if (category == BuiltinFunctionCategory::list) {
        return evaluate_list_builtin(function, arguments);
    }

    if (category == BuiltinFunctionCategory::list_generation) {
        return evaluate_list_generation_builtin(function, arguments);
    }

    throw EvaluationError("unknown function");
}

}  // namespace console_calc::detail
