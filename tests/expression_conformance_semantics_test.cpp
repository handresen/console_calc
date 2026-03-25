#include "expression_conformance_checks.h"

#include <cmath>
#include <exception>
#include <variant>

#include "expression_conformance_test_support.h"

namespace console_calc::test {

bool expect_expression_semantics(ExpressionParser& parser) {
    const Value first_list = parser.evaluate_value("first({1, 2, 3}, 2)");
    const auto* first_values = std::get_if<ListValue>(&first_list);
    if (first_values == nullptr || first_values->size() != 2 ||
        !almost_equal(scalar_to_double((*first_values)[0]), 1.0) ||
        !almost_equal(scalar_to_double((*first_values)[1]), 2.0)) {
        return false;
    }

    const Value dropped_list = parser.evaluate_value("drop({1, 2, 3, 4}, 2)");
    const auto* dropped_values = std::get_if<ListValue>(&dropped_list);
    if (dropped_values == nullptr || dropped_values->size() != 2 ||
        !almost_equal(scalar_to_double((*dropped_values)[0]), 3.0) ||
        !almost_equal(scalar_to_double((*dropped_values)[1]), 4.0)) {
        return false;
    }

    if (!almost_equal(parser.evaluate("first({2}, 1) + 3"), 5.0) ||
        !almost_equal(parser.evaluate("drop({1, 2, 3}, 2) + 4"), 7.0) ||
        !almost_equal(parser.evaluate("{1, 2, 3}[1]"), 2.0) ||
        !almost_equal(parser.evaluate("range(10, 3)[2]"), 12.0) ||
        !almost_equal(parser.evaluate("sin({0})"), 0.0)) {
        return false;
    }

    const Value mapped_list = parser.evaluate_value("map({0, 1.5707963267948966}, sin(_))");
    const auto* mapped_values = std::get_if<ListValue>(&mapped_list);
    if (mapped_values == nullptr || mapped_values->size() != 2 ||
        !almost_equal(scalar_to_double((*mapped_values)[0]), 0.0) ||
        !almost_equal(scalar_to_double((*mapped_values)[1]), 1.0)) {
        return false;
    }

    const Value filtered_list = parser.evaluate_value("list_where({1, 2, 3, 4, 5}, _ <= 3)");
    const auto* filtered_values = std::get_if<ListValue>(&filtered_list);
    const Value odd_values = parser.evaluate_value("list_where({1, 2, 3, 4, 5}, _ % 2)");
    const auto* odd_filtered_values = std::get_if<ListValue>(&odd_values);
    if (filtered_values == nullptr || filtered_values->size() != 3 ||
        !almost_equal(scalar_to_double((*filtered_values)[0]), 1.0) ||
        !almost_equal(scalar_to_double((*filtered_values)[1]), 2.0) ||
        !almost_equal(scalar_to_double((*filtered_values)[2]), 3.0) ||
        odd_filtered_values == nullptr || odd_filtered_values->size() != 3 ||
        !almost_equal(scalar_to_double((*odd_filtered_values)[0]), 1.0) ||
        !almost_equal(scalar_to_double((*odd_filtered_values)[1]), 3.0) ||
        !almost_equal(scalar_to_double((*odd_filtered_values)[2]), 5.0)) {
        return false;
    }

    const Value generated_range = parser.evaluate_value("range(2, 4, 3)");
    const auto* generated_values = std::get_if<ListValue>(&generated_range);
    if (generated_values == nullptr || generated_values->size() != 4 ||
        !almost_equal(scalar_to_double((*generated_values)[0]), 2.0) ||
        !almost_equal(scalar_to_double((*generated_values)[1]), 5.0) ||
        !almost_equal(scalar_to_double((*generated_values)[2]), 8.0) ||
        !almost_equal(scalar_to_double((*generated_values)[3]), 11.0)) {
        return false;
    }

    const Value generated_geom = parser.evaluate_value("geom(3, 4, 3)");
    const auto* geometric_values = std::get_if<ListValue>(&generated_geom);
    if (geometric_values == nullptr || geometric_values->size() != 4 ||
        !almost_equal(scalar_to_double((*geometric_values)[0]), 3.0) ||
        !almost_equal(scalar_to_double((*geometric_values)[1]), 9.0) ||
        !almost_equal(scalar_to_double((*geometric_values)[2]), 27.0) ||
        !almost_equal(scalar_to_double((*geometric_values)[3]), 81.0)) {
        return false;
    }

    const Value generated_repeat = parser.evaluate_value("repeat(2.5, 3)");
    const auto* repeated_values = std::get_if<ListValue>(&generated_repeat);
    if (repeated_values == nullptr || repeated_values->size() != 3 ||
        !almost_equal(scalar_to_double((*repeated_values)[0]), 2.5) ||
        !almost_equal(scalar_to_double((*repeated_values)[1]), 2.5) ||
        !almost_equal(scalar_to_double((*repeated_values)[2]), 2.5)) {
        return false;
    }

    const Value generated_linspace = parser.evaluate_value("linspace(1, 4, 4)");
    const auto* spaced_values = std::get_if<ListValue>(&generated_linspace);
    if (spaced_values == nullptr || spaced_values->size() != 4 ||
        !almost_equal(scalar_to_double((*spaced_values)[0]), 1.0) ||
        !almost_equal(scalar_to_double((*spaced_values)[1]), 2.0) ||
        !almost_equal(scalar_to_double((*spaced_values)[2]), 3.0) ||
        !almost_equal(scalar_to_double((*spaced_values)[3]), 4.0)) {
        return false;
    }

    const Value generated_powers = parser.evaluate_value("powers(-1, 4)");
    const auto* power_values = std::get_if<ListValue>(&generated_powers);
    if (power_values == nullptr || power_values->size() != 4 ||
        !almost_equal(scalar_to_double((*power_values)[0]), 1.0) ||
        !almost_equal(scalar_to_double((*power_values)[1]), -1.0) ||
        !almost_equal(scalar_to_double((*power_values)[2]), 1.0) ||
        !almost_equal(scalar_to_double((*power_values)[3]), -1.0)) {
        return false;
    }

    const Value added_lists = parser.evaluate_value("list_add({2, 3, 4}, {5, 6, 7})");
    const auto* added_values = std::get_if<ListValue>(&added_lists);
    const Value multiplied_lists = parser.evaluate_value("list_mul({2, 3, 4}, {5, 6, 7})");
    const auto* multiplied_values = std::get_if<ListValue>(&multiplied_lists);
    const Value divided_lists = parser.evaluate_value("list_div({8, 9, 10}, {2, 3, 5})");
    const auto* divided_values = std::get_if<ListValue>(&divided_lists);
    const Value subtracted_lists = parser.evaluate_value("list_sub({8, 9, 10}, {2, 3, 5})");
    const auto* subtracted_values = std::get_if<ListValue>(&subtracted_lists);
    if (added_values == nullptr || multiplied_values == nullptr || divided_values == nullptr ||
        subtracted_values == nullptr || added_values->size() != 3 || multiplied_values->size() != 3 ||
        divided_values->size() != 3 || subtracted_values->size() != 3 ||
        !almost_equal(scalar_to_double((*added_values)[0]), 7.0) ||
        !almost_equal(scalar_to_double((*added_values)[1]), 9.0) ||
        !almost_equal(scalar_to_double((*added_values)[2]), 11.0) ||
        !almost_equal(scalar_to_double((*multiplied_values)[0]), 10.0) ||
        !almost_equal(scalar_to_double((*multiplied_values)[1]), 18.0) ||
        !almost_equal(scalar_to_double((*multiplied_values)[2]), 28.0) ||
        !almost_equal(scalar_to_double((*divided_values)[0]), 4.0) ||
        !almost_equal(scalar_to_double((*divided_values)[1]), 3.0) ||
        !almost_equal(scalar_to_double((*divided_values)[2]), 2.0) ||
        !almost_equal(scalar_to_double((*subtracted_values)[0]), 6.0) ||
        !almost_equal(scalar_to_double((*subtracted_values)[1]), 6.0) ||
        !almost_equal(scalar_to_double((*subtracted_values)[2]), 5.0)) {
        return false;
    }

    const Value reduced_list = parser.evaluate_value("reduce({2, 3, 4}, *)");
    const Value absolute_value = parser.evaluate_value("abs(-3)");
    const Value square_root_value = parser.evaluate_value("sqrt(9)");
    if (!std::holds_alternative<std::int64_t>(reduced_list) || std::get<std::int64_t>(reduced_list) != 24 ||
        !std::holds_alternative<std::int64_t>(absolute_value) || std::get<std::int64_t>(absolute_value) != 3 ||
        !std::holds_alternative<double>(square_root_value) ||
        !almost_equal(std::get<double>(square_root_value), 3.0)) {
        return false;
    }

    const Value position_value = parser.evaluate_value("pos(60, 10)");
    const auto* position = std::get_if<PositionValue>(&position_value);
    if (position == nullptr || !almost_equal(position->latitude_deg, 60.0) ||
        !almost_equal(position->longitude_deg, 10.0) ||
        !almost_equal(parser.evaluate("lat(pos(60, 10))"), 60.0) ||
        !almost_equal(parser.evaluate("lon(pos(60, 10))"), 10.0) ||
        !almost_equal(parser.evaluate("dist(pos(0, 0), pos(0, 1))"), 111319.4907932264, 1e-6) ||
        !almost_equal(parser.evaluate("dist({pos(0, 0), pos(0, 1), pos(0, 2)})"),
                      222638.9815864528, 1e-6) ||
        !almost_equal(parser.evaluate("dist({pos(60, 10)})"), 0.0, 1e-12) ||
        !almost_equal(parser.evaluate("bearing(pos(0, 0), pos(0, 1))"), 90.0, 1e-9) ||
        !almost_equal(parser.evaluate("lat(br_to_pos(pos(0, 0), 90, 111319.4907932264))"), 0.0, 1e-8) ||
        !almost_equal(parser.evaluate("lon(br_to_pos(pos(0, 0), 90, 111319.4907932264))"), 1.0, 1e-8)) {
        return false;
    }

    const Value ranged_map = parser.evaluate_value("map({10, 20, 30, 40, 50}, _ + 1, 1, 2, 2)");
    const auto* ranged_values = std::get_if<ListValue>(&ranged_map);
    const Value default_end_map = parser.evaluate_value("map({10, 20, 30, 40, 50}, _ + 1, 2)");
    const auto* default_end_values = std::get_if<ListValue>(&default_end_map);
    const Value ranged_map_at =
        parser.evaluate_value("map_at({10, 20, 30, 40, 50}, _ + 1, 1, 2, 2)");
    const auto* ranged_map_at_values = std::get_if<ListValue>(&ranged_map_at);
    if (ranged_values == nullptr || default_end_values == nullptr || ranged_map_at_values == nullptr ||
        ranged_values->size() != 2 || default_end_values->size() != 3 || ranged_map_at_values->size() != 5 ||
        !almost_equal(scalar_to_double((*ranged_values)[0]), 21.0) ||
        !almost_equal(scalar_to_double((*ranged_values)[1]), 41.0) ||
        !almost_equal(scalar_to_double((*default_end_values)[0]), 31.0) ||
        !almost_equal(scalar_to_double((*default_end_values)[1]), 41.0) ||
        !almost_equal(scalar_to_double((*default_end_values)[2]), 51.0) ||
        !almost_equal(scalar_to_double((*ranged_map_at_values)[0]), 10.0) ||
        !almost_equal(scalar_to_double((*ranged_map_at_values)[1]), 21.0) ||
        !almost_equal(scalar_to_double((*ranged_map_at_values)[2]), 30.0) ||
        !almost_equal(scalar_to_double((*ranged_map_at_values)[3]), 41.0) ||
        !almost_equal(scalar_to_double((*ranged_map_at_values)[4]), 50.0)) {
        return false;
    }

    const Value paired_positions = parser.evaluate_value("to_poslist({60, 10, 61, 11})");
    const auto* paired_values = std::get_if<PositionListValue>(&paired_positions);
    const Value flattened_positions = parser.evaluate_value("to_list({pos(60, 10), pos(61, 11)})");
    const auto* flattened_values = std::get_if<ListValue>(&flattened_positions);
    const Value empty_position_list = parser.evaluate_value("to_poslist({})");
    const auto* empty_positions = std::get_if<PositionListValue>(&empty_position_list);
    const Value empty_scalar_list = parser.evaluate_value("to_list(to_poslist({}))");
    const auto* empty_scalars = std::get_if<ListValue>(&empty_scalar_list);
    const Value filled_positions = parser.evaluate_value("fill(pos(60, 10), 2)");
    const auto* position_values = std::get_if<PositionListValue>(&filled_positions);
    const Value densified_positions =
        parser.evaluate_value("densify_path({pos(0, 0), pos(0, 1)}, 2)");
    const auto* densified_values = std::get_if<PositionListValue>(&densified_positions);
    const Value simplified_positions = parser.evaluate_value(
        "simplify_path(densify_path({pos(0, 0), pos(0, 1)}, 2), 1.0)");
    const auto* simplified_values = std::get_if<PositionListValue>(&simplified_positions);
    const Value compressed_positions = parser.evaluate_value(
        "compress_path(densify_path({pos(0, 0), pos(0, 1)}, 4), 2)");
    const auto* compressed_values = std::get_if<PositionListValue>(&compressed_positions);
    const Value zigzag_compressed_positions = parser.evaluate_value(
        "compress_path(densify_path(to_poslist({60, 10, 61, 11, 62, 10, 63, 11}), 10), 4)");
    const auto* zigzag_compressed_values =
        std::get_if<PositionListValue>(&zigzag_compressed_positions);
    const Value indexed_position = parser.evaluate_value("to_poslist({60, 10, 61, 11})[1]");
    const auto* indexed_position_value = std::get_if<PositionValue>(&indexed_position);
    if (paired_values == nullptr || paired_values->size() != 2 || flattened_values == nullptr ||
        flattened_values->size() != 4 || empty_positions == nullptr || !empty_positions->empty() ||
        empty_scalars == nullptr || !empty_scalars->empty() || position_values == nullptr ||
        position_values->size() != 2 || densified_values == nullptr ||
        densified_values->size() != 4 ||
        simplified_values == nullptr || simplified_values->size() != 2 ||
        compressed_values == nullptr || compressed_values->size() != 2 ||
        zigzag_compressed_values == nullptr || zigzag_compressed_values->size() != 4 ||
        !almost_equal(densified_values->front().longitude_deg, 0.0, 1e-12) ||
        !almost_equal(densified_values->back().longitude_deg, 1.0, 1e-12) ||
        !almost_equal(simplified_values->front().longitude_deg, 0.0, 1e-12) ||
        !almost_equal(simplified_values->back().longitude_deg, 1.0, 1e-12) ||
        !almost_equal(compressed_values->front().longitude_deg, 0.0, 1e-12) ||
        !almost_equal(compressed_values->back().longitude_deg, 1.0, 1e-12) ||
        !almost_equal((*zigzag_compressed_values)[0].latitude_deg, 60.0, 1e-9) ||
        !almost_equal((*zigzag_compressed_values)[0].longitude_deg, 10.0, 1e-9) ||
        !almost_equal((*zigzag_compressed_values)[1].latitude_deg, 61.0, 1e-9) ||
        !almost_equal((*zigzag_compressed_values)[1].longitude_deg, 11.0, 1e-9) ||
        !almost_equal((*zigzag_compressed_values)[2].latitude_deg, 62.0, 1e-9) ||
        !almost_equal((*zigzag_compressed_values)[2].longitude_deg, 10.0, 1e-9) ||
        !almost_equal((*zigzag_compressed_values)[3].latitude_deg, 63.0, 1e-9) ||
        !almost_equal((*zigzag_compressed_values)[3].longitude_deg, 11.0, 1e-9) ||
        !almost_equal(parser.evaluate("dist(densify_path({pos(0, 0), pos(0, 1)}, 2))"),
                      111319.4907932264, 1e-6) ||
        !almost_equal(
            parser.evaluate("dist(simplify_path(densify_path({pos(0, 0), pos(0, 1)}, 2), 1.0))"),
            111319.4907932264, 1e-6) ||
        !almost_equal(
            parser.evaluate("dist(compress_path(densify_path({pos(0, 0), pos(0, 1)}, 4), 2))"),
            111319.4907932264, 1e-6) ||
        indexed_position_value == nullptr ||
        !almost_equal(indexed_position_value->latitude_deg, 61.0) ||
        !almost_equal(indexed_position_value->longitude_deg, 11.0)) {
        return false;
    }

    const Value integer_sum = parser.evaluate_value("1 + 2");
    const Value floating_division = parser.evaluate_value("1 / 2");
    const Value integer_sum_list = parser.evaluate_value("sum({1, 2, 3})");
    const Value mixed_sum_list = parser.evaluate_value("sum({1, 2.5})");
    const Value integer_modulo = parser.evaluate_value("7 % 3");
    const Value floating_modulo = parser.evaluate_value("7.5 % 2");
    const Value integer_length = parser.evaluate_value("len({1, 2, 3})");
    const Value position_length = parser.evaluate_value("len({pos(0, 0), pos(0, 1)})");
    const Value bitwise_not = parser.evaluate_value("~5");
    const Value bitwise_and_fn = parser.evaluate_value("and(6, 3)");
    const Value bitwise_or_fn = parser.evaluate_value("or(6, 3)");
    const Value bitwise_xor_fn = parser.evaluate_value("xor(6, 3)");
    const Value bitwise_nand_fn = parser.evaluate_value("nand(6, 3)");
    const Value bitwise_nor_fn = parser.evaluate_value("nor(6, 3)");
    const Value shift_left_fn = parser.evaluate_value("shl(3, 4)");
    const Value shift_right_fn = parser.evaluate_value("shr(48, 4)");
    const Value equal_true = parser.evaluate_value("3 = 3");
    const Value equal_false = parser.evaluate_value("3 = 4");
    const Value less_true = parser.evaluate_value("2 < 3");
    const Value less_false = parser.evaluate_value("3 < 2");
    const Value less_equal_true = parser.evaluate_value("3 <= 3");
    const Value greater_true = parser.evaluate_value("4 > 1");
    const Value greater_equal_false = parser.evaluate_value("2 >= 5");
    if (!std::holds_alternative<std::int64_t>(integer_sum) || std::get<std::int64_t>(integer_sum) != 3 ||
        !std::holds_alternative<double>(floating_division) || !almost_equal(std::get<double>(floating_division), 0.5) ||
        !std::holds_alternative<std::int64_t>(integer_sum_list) || std::get<std::int64_t>(integer_sum_list) != 6 ||
        !std::holds_alternative<double>(mixed_sum_list) || !almost_equal(std::get<double>(mixed_sum_list), 3.5) ||
        !std::holds_alternative<std::int64_t>(integer_modulo) || std::get<std::int64_t>(integer_modulo) != 1 ||
        !std::holds_alternative<double>(floating_modulo) || !almost_equal(std::get<double>(floating_modulo), 1.5) ||
        !std::holds_alternative<std::int64_t>(integer_length) || std::get<std::int64_t>(integer_length) != 3 ||
        !std::holds_alternative<std::int64_t>(position_length) || std::get<std::int64_t>(position_length) != 2 ||
        !std::holds_alternative<std::int64_t>(bitwise_not) || std::get<std::int64_t>(bitwise_not) != -6 ||
        !std::holds_alternative<std::int64_t>(bitwise_and_fn) || std::get<std::int64_t>(bitwise_and_fn) != 2 ||
        !std::holds_alternative<std::int64_t>(bitwise_or_fn) || std::get<std::int64_t>(bitwise_or_fn) != 7 ||
        !std::holds_alternative<std::int64_t>(bitwise_xor_fn) || std::get<std::int64_t>(bitwise_xor_fn) != 5 ||
        !std::holds_alternative<std::int64_t>(bitwise_nand_fn) || std::get<std::int64_t>(bitwise_nand_fn) != -3 ||
        !std::holds_alternative<std::int64_t>(bitwise_nor_fn) || std::get<std::int64_t>(bitwise_nor_fn) != -8 ||
        !std::holds_alternative<std::int64_t>(shift_left_fn) || std::get<std::int64_t>(shift_left_fn) != 48 ||
        !std::holds_alternative<std::int64_t>(shift_right_fn) || std::get<std::int64_t>(shift_right_fn) != 3 ||
        !std::holds_alternative<std::int64_t>(equal_true) || std::get<std::int64_t>(equal_true) != 1 ||
        !std::holds_alternative<std::int64_t>(equal_false) || std::get<std::int64_t>(equal_false) != 0 ||
        !std::holds_alternative<std::int64_t>(less_true) || std::get<std::int64_t>(less_true) != 1 ||
        !std::holds_alternative<std::int64_t>(less_false) || std::get<std::int64_t>(less_false) != 0 ||
        !std::holds_alternative<std::int64_t>(less_equal_true) ||
        std::get<std::int64_t>(less_equal_true) != 1 ||
        !std::holds_alternative<std::int64_t>(greater_true) ||
        std::get<std::int64_t>(greater_true) != 1 ||
        !std::holds_alternative<std::int64_t>(greater_equal_false) ||
        std::get<std::int64_t>(greater_equal_false) != 0 ||
        !almost_equal(parser.evaluate("1 + 2 = 3"), 1.0) ||
        !almost_equal(parser.evaluate("1 + 2 = 4"), 0.0) ||
        !almost_equal(parser.evaluate("1 + 2 < 4"), 1.0) ||
        !almost_equal(parser.evaluate("1 + 2 < 3"), 0.0) ||
        !almost_equal(parser.evaluate("5 & 3 < 1"), 0.0)) {
        return false;
    }

    const Value elapsed = parser.evaluate_value("timed_loop(1 + 2, 3)");
    const Value zero_elapsed = parser.evaluate_value("timed_loop(1 + 2, 0)");
    const Value filled = parser.evaluate_value("fill(1 + 2, 3)");
    const Value default_random = parser.evaluate_value("rand()");
    const Value bounded_random = parser.evaluate_value("rand(5)");
    const Value ranged_random = parser.evaluate_value("rand(2, 5)");
    if (!std::holds_alternative<double>(elapsed) || !std::isfinite(std::get<double>(elapsed)) ||
        std::get<double>(elapsed) < 0.0 || !std::holds_alternative<double>(zero_elapsed) ||
        !std::isfinite(std::get<double>(zero_elapsed)) || std::get<double>(zero_elapsed) < 0.0 ||
        !std::holds_alternative<ListValue>(filled) ||
        !std::holds_alternative<double>(default_random) || std::get<double>(default_random) < 0.0 ||
        std::get<double>(default_random) >= 1.0 || !std::holds_alternative<double>(bounded_random) ||
        std::get<double>(bounded_random) < 0.0 || std::get<double>(bounded_random) >= 5.0 ||
        !std::holds_alternative<double>(ranged_random) || std::get<double>(ranged_random) < 2.0 ||
        std::get<double>(ranged_random) >= 5.0) {
        return false;
    }

    const char* invalid_expressions[] = {
        "sin({1, 2})",
        "sum({pos(60, 10), pos(61, 11)})",
        "to_poslist({60, 10, 61})",
        "to_list({60, 10})",
        "map({1, 2, 3}, _ + 1, 0, 0)",
        "list_where(1, _ + 1)",
        "{1, pos(60, 10)}",
        "pow({2, 3}, 2)",
        "{1, 2}[2]",
        "{1, 2}[-1]",
        "{1, 2}[1.5]",
        "1[0]",
        "~1.5",
        "xor(1.5, 1)",
        "shl(1, -1)",
        "shr(1, 64)",
        "lat(1)",
        "timed_loop(1 + 2, -1)",
        "rand(1, 1)",
        "rand(5, 2)",
    };
    for (const char* expression : invalid_expressions) {
        try {
            (void)parser.evaluate(expression);
            return false;
        } catch (const std::invalid_argument&) {
        } catch (const std::exception&) {
            return false;
        }
    }

    return true;
}

}  // namespace console_calc::test
