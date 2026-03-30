#include "expression_conformance_checks.h"

#include <cmath>
#include <exception>
#include <string>
#include <variant>

#include "expression_conformance_test_support.h"

namespace console_calc::test {

bool expect_expression_semantics(ExpressionParser& parser) {
    const auto require = [](bool condition, const char* label) {
        if (!condition) {
            std::cerr << "semantic check failed: " << label << '\n';
        }
        return condition;
    };
    const auto nest = [](std::string expression, const std::string& wrapper_template, int count) {
        for (int index = 0; index < count; ++index) {
            std::string wrapper = wrapper_template;
            const std::size_t placeholder = wrapper.find("{}");
            if (placeholder == std::string::npos) {
                throw std::runtime_error("missing wrapper placeholder");
            }
            wrapper.replace(placeholder, 2, expression);
            expression = std::move(wrapper);
        }
        return expression;
    };

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
        !almost_equal(parser.evaluate("{{1, 2}, {3, 4}}[1][0]"), 3.0) ||
        !almost_equal(parser.evaluate("len({{1, 2}, {3, 4}})"), 2.0) ||
        !almost_equal(parser.evaluate("len({{pos(0, 0), pos(0, 1)}, {pos(1, 1)}})"), 2.0) ||
        !almost_equal(parser.evaluate("lat({{pos(0, 0), pos(0, 1)}, {pos(1, 1)}}[1][0])"), 1.0) ||
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
    const Value scaled_list_left = parser.evaluate_value("2 * {3, 4}");
    const auto* scaled_left_values = std::get_if<ListValue>(&scaled_list_left);
    const Value scaled_list_right = parser.evaluate_value("{3, 4} * 2");
    const auto* scaled_right_values = std::get_if<ListValue>(&scaled_list_right);
    const Value divided_by_scalar = parser.evaluate_value("{8, 10} / 2");
    const auto* divided_by_scalar_values = std::get_if<ListValue>(&divided_by_scalar);
    const Value negated_list = parser.evaluate_value("-{3, -4}");
    const auto* negated_list_values = std::get_if<ListValue>(&negated_list);
    const Value divided_lists = parser.evaluate_value("list_div({8, 9, 10}, {2, 3, 5})");
    const auto* divided_values = std::get_if<ListValue>(&divided_lists);
    const Value subtracted_lists = parser.evaluate_value("list_sub({8, 9, 10}, {2, 3, 5})");
    const auto* subtracted_values = std::get_if<ListValue>(&subtracted_lists);
    if (added_values == nullptr || multiplied_values == nullptr || divided_values == nullptr ||
        subtracted_values == nullptr || scaled_left_values == nullptr ||
        scaled_right_values == nullptr || divided_by_scalar_values == nullptr ||
        negated_list_values == nullptr || added_values->size() != 3 ||
        multiplied_values->size() != 3 ||
        divided_values->size() != 3 || subtracted_values->size() != 3 ||
        scaled_left_values->size() != 2 || scaled_right_values->size() != 2 ||
        divided_by_scalar_values->size() != 2 || negated_list_values->size() != 2 ||
        !almost_equal(scalar_to_double((*added_values)[0]), 7.0) ||
        !almost_equal(scalar_to_double((*added_values)[1]), 9.0) ||
        !almost_equal(scalar_to_double((*added_values)[2]), 11.0) ||
        !almost_equal(scalar_to_double((*multiplied_values)[0]), 10.0) ||
        !almost_equal(scalar_to_double((*multiplied_values)[1]), 18.0) ||
        !almost_equal(scalar_to_double((*multiplied_values)[2]), 28.0) ||
        !almost_equal(scalar_to_double((*scaled_left_values)[0]), 6.0) ||
        !almost_equal(scalar_to_double((*scaled_left_values)[1]), 8.0) ||
        !almost_equal(scalar_to_double((*scaled_right_values)[0]), 6.0) ||
        !almost_equal(scalar_to_double((*scaled_right_values)[1]), 8.0) ||
        !almost_equal(scalar_to_double((*divided_by_scalar_values)[0]), 4.0) ||
        !almost_equal(scalar_to_double((*divided_by_scalar_values)[1]), 5.0) ||
        !almost_equal(scalar_to_double((*negated_list_values)[0]), -3.0) ||
        !almost_equal(scalar_to_double((*negated_list_values)[1]), 4.0) ||
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
    const Value offset_right_positions =
        parser.evaluate_value("offset_path({pos(0, 0), pos(0, 1)}, 1000, 0)");
    const auto* offset_right_values = std::get_if<PositionListValue>(&offset_right_positions);
    const Value offset_forward_positions =
        parser.evaluate_value("offset_path({pos(0, 0), pos(0, 1)}, 0, 1000)");
    const auto* offset_forward_values = std::get_if<PositionListValue>(&offset_forward_positions);
    const double repeated_offset_middle_distance = parser.evaluate(
        "dist(offset_path(offset_path({pos(0, 0), pos(0, 1), pos(1, 1)}, 10000, 10000), 10000, 10000)[1],"
        " offset_path({pos(0, 0), pos(0, 1), pos(1, 1)}, 20000, 20000)[1])");
    const double repeated_offset_path_length_delta = std::fabs(
        parser.evaluate("dist(offset_path(offset_path({pos(0, 0), pos(0, 1), pos(1, 1)}, 10000, 10000), 10000, 10000))") -
        parser.evaluate("dist(offset_path({pos(0, 0), pos(0, 1), pos(1, 1)}, 20000, 20000))"));
    const double rotated_center_distance = parser.evaluate(
        "dist(rotate_path({pos(0, 0), pos(0, 1), pos(1, 1)}, 90, 1)[1], pos(0, 1))");
    const double rotated_first_radius_delta = std::fabs(
        parser.evaluate("dist(rotate_path({pos(0, 0), pos(0, 1), pos(1, 1)}, 90, 1)[0], pos(0, 1))") -
        parser.evaluate("dist(pos(0, 0), pos(0, 1))"));
    const double rotated_second_radius_delta = std::fabs(
        parser.evaluate("dist(rotate_path({pos(0, 0), pos(0, 1), pos(1, 1)}, 90, 1)[2], pos(0, 1))") -
        parser.evaluate("dist(pos(1, 1), pos(0, 1))"));
    const double repeated_rotation_middle_distance = parser.evaluate(
        "dist(rotate_path(rotate_path({pos(0, 0), pos(0, 1), pos(1, 1)}, 45, 1), 45, 1)[0],"
        " rotate_path({pos(0, 0), pos(0, 1), pos(1, 1)}, 90, 1)[0])");
    const Value scaled_positions = parser.evaluate_value(
        "scale_path({pos(0, 0), pos(0, 1), pos(1, 1)}, 2)");
    const auto* scaled_values = std::get_if<PositionListValue>(&scaled_positions);
    const double scale_identity_delta = parser.evaluate(
        "dist(scale_path({pos(0, 0), pos(0, 1), pos(1, 1)}, 1)[2], pos(1, 1))");
    const double scale_round_trip_first_drift = parser.evaluate(
        "dist(scale_path(scale_path({pos(0, 0), pos(0, 1), pos(1, 1)}, 0.5), 2)[0], pos(0, 0))");
    const double scale_round_trip_center_drift = parser.evaluate(
        "dist(scale_path(scale_path({pos(0, 0), pos(0, 1), pos(1, 1)}, 0.5), 2)[1], pos(0, 1))");
    const double scale_round_trip_second_drift = parser.evaluate(
        "dist(scale_path(scale_path({pos(0, 0), pos(0, 1), pos(1, 1)}, 0.5), 2)[2], pos(1, 1))");
    const Value scaled_closed_positions = parser.evaluate_value(
        "scale_path({pos(0, 0), pos(0, 1), pos(1, 1), pos(0, 0)}, 1.5)");
    const auto* scaled_closed_values = std::get_if<PositionListValue>(&scaled_closed_positions);
    const std::string rotation_base = "{pos(0, 0), pos(0, 1), pos(1, 1)}";
    const std::string rotated_full_cycle =
        nest(rotation_base, "rotate_path({}, 1, 1)", 3600);
    const double rotate_cycle_center_drift = parser.evaluate(
        "dist(" + rotated_full_cycle + "[1], pos(0, 1))");
    const double rotate_cycle_first_drift = parser.evaluate(
        "dist(" + rotated_full_cycle + "[0], pos(0, 0))");
    const double rotate_cycle_second_drift = parser.evaluate(
        "dist(" + rotated_full_cycle + "[2], pos(1, 1))");
    const std::string offset_base = "{pos(0, 0), pos(0, 1), pos(1, 1)}";
    const std::string offset_round_trip = nest(
        nest(offset_base, "offset_path({}, 1000, 1000)", 1000),
        "offset_path({}, -1000, -1000)", 1000);
    const double offset_round_trip_first_drift = parser.evaluate(
        "dist(" + offset_round_trip + "[0], pos(0, 0))");
    const double offset_round_trip_center_drift = parser.evaluate(
        "dist(" + offset_round_trip + "[1], pos(0, 1))");
    const double offset_round_trip_second_drift = parser.evaluate(
        "dist(" + offset_round_trip + "[2], pos(1, 1))");
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
    const Value multi_position_distances = parser.evaluate_value(
        "dist({{pos(0, 0), pos(0, 1)}, {pos(0, 2), pos(0, 3)}})");
    const auto* multi_position_distance_values = std::get_if<ListValue>(&multi_position_distances);
    const Value flattened_multi_positions = parser.evaluate_value(
        "to_list({{pos(0, 0), pos(0, 1)}, {pos(0, 2), pos(0, 3)}})");
    const auto* flattened_multi_position_values = std::get_if<MultiListValue>(&flattened_multi_positions);
    const Value multi_densified_positions = parser.evaluate_value(
        "densify_path({{pos(0, 0), pos(0, 1)}, {pos(0, 2), pos(0, 3)}}, 2)");
    const auto* multi_densified_values = std::get_if<MultiPositionListValue>(&multi_densified_positions);
    const Value multi_offset_positions = parser.evaluate_value(
        "offset_path({{pos(0, 0), pos(0, 1)}, {pos(0, 2), pos(0, 3)}}, 1000, 0)");
    const auto* multi_offset_values = std::get_if<MultiPositionListValue>(&multi_offset_positions);
    const Value multi_rotated_positions = parser.evaluate_value(
        "rotate_path({{pos(0, 0), pos(0, 1), pos(1, 1)}, {pos(0, 2), pos(0, 3), pos(1, 3)}}, 90, 1)");
    const auto* multi_rotated_values = std::get_if<MultiPositionListValue>(&multi_rotated_positions);
    const Value multi_scaled_positions = parser.evaluate_value(
        "scale_path({{pos(0, 0), pos(0, 1), pos(1, 1)}, {pos(0, 2), pos(0, 3), pos(1, 3)}}, 2)");
    const auto* multi_scaled_values = std::get_if<MultiPositionListValue>(&multi_scaled_positions);
    const Value multi_simplified_positions = parser.evaluate_value(
        "simplify_path(densify_path({{pos(0, 0), pos(0, 1)}, {pos(0, 2), pos(0, 3)}}, 2), 1.0)");
    const auto* multi_simplified_values = std::get_if<MultiPositionListValue>(&multi_simplified_positions);
    const Value multi_compressed_positions = parser.evaluate_value(
        "compress_path(densify_path({{pos(0, 0), pos(0, 1)}, {pos(0, 2), pos(0, 3)}}, 4), 2)");
    const auto* multi_compressed_values = std::get_if<MultiPositionListValue>(&multi_compressed_positions);
    const double high_lat_segment_distance =
        parser.evaluate("dist(pos(70, 10), pos(70, 11))");
    const double high_lat_segment_bearing =
        parser.evaluate("bearing(pos(70, 10), pos(70, 11))");
    const double high_lat_round_trip_lat = parser.evaluate(
        "lat(br_to_pos(pos(70, 10), bearing(pos(70, 10), pos(70, 11)), dist(pos(70, 10), pos(70, 11))))");
    const double high_lat_round_trip_lon = parser.evaluate(
        "lon(br_to_pos(pos(70, 10), bearing(pos(70, 10), pos(70, 11)), dist(pos(70, 10), pos(70, 11))))");
    const Value high_lat_densified_positions =
        parser.evaluate_value("densify_path({pos(70, 10), pos(70, 11)}, 2)");
    const auto* high_lat_densified_values = std::get_if<PositionListValue>(&high_lat_densified_positions);
    const Value high_lat_offset_right_positions =
        parser.evaluate_value("offset_path({pos(70, 10), pos(70, 11)}, 1000, 0)");
    const auto* high_lat_offset_right_values = std::get_if<PositionListValue>(&high_lat_offset_right_positions);
    const Value high_lat_offset_forward_positions =
        parser.evaluate_value("offset_path({pos(70, 10), pos(70, 11)}, 0, 1000)");
    const auto* high_lat_offset_forward_values = std::get_if<PositionListValue>(&high_lat_offset_forward_positions);
    const double high_lat_offset_distance =
        parser.evaluate("dist(offset_path({pos(70, 10), pos(70, 11)}, 1000, 0))");
    const Value high_lat_rotated_positions =
        parser.evaluate_value("rotate_path({pos(70, 10), pos(70, 11), pos(71, 11)}, 90, 1)");
    const auto* high_lat_rotated_values = std::get_if<PositionListValue>(&high_lat_rotated_positions);
    const double high_lat_rotated_center_distance = parser.evaluate(
        "dist(rotate_path({pos(70, 10), pos(70, 11), pos(71, 11)}, 90, 1)[1], pos(70, 11))");
    const double high_lat_rotated_first_radius_delta = std::fabs(
        parser.evaluate("dist(rotate_path({pos(70, 10), pos(70, 11), pos(71, 11)}, 90, 1)[0], pos(70, 11))") -
        parser.evaluate("dist(pos(70, 10), pos(70, 11))"));
    const double high_lat_rotated_second_radius_delta = std::fabs(
        parser.evaluate("dist(rotate_path({pos(70, 10), pos(70, 11), pos(71, 11)}, 90, 1)[2], pos(70, 11))") -
        parser.evaluate("dist(pos(71, 11), pos(70, 11))"));
    const Value high_lat_scaled_positions =
        parser.evaluate_value("scale_path({pos(70, 10), pos(70, 11), pos(71, 11)}, 2)");
    const auto* high_lat_scaled_values = std::get_if<PositionListValue>(&high_lat_scaled_positions);
    const double high_lat_scale_identity_delta = parser.evaluate(
        "dist(scale_path({pos(70, 10), pos(70, 11), pos(71, 11)}, 1)[2], pos(71, 11))");
    const double high_lat_scale_round_trip_first_drift = parser.evaluate(
        "dist(scale_path(scale_path({pos(70, 10), pos(70, 11), pos(71, 11)}, 0.5), 2)[0], pos(70, 10))");
    const double high_lat_scale_round_trip_center_drift = parser.evaluate(
        "dist(scale_path(scale_path({pos(70, 10), pos(70, 11), pos(71, 11)}, 0.5), 2)[1], pos(70, 11))");
    const double high_lat_scale_round_trip_second_drift = parser.evaluate(
        "dist(scale_path(scale_path({pos(70, 10), pos(70, 11), pos(71, 11)}, 0.5), 2)[2], pos(71, 11))");
    const Value high_lat_simplified_positions = parser.evaluate_value(
        "simplify_path(densify_path({pos(70, 10), pos(70, 11)}, 2), 1.0)");
    const auto* high_lat_simplified_values = std::get_if<PositionListValue>(&high_lat_simplified_positions);
    const Value high_lat_compressed_positions = parser.evaluate_value(
        "compress_path(densify_path({pos(70, 10), pos(70, 11)}, 4), 2)");
    const auto* high_lat_compressed_values = std::get_if<PositionListValue>(&high_lat_compressed_positions);
    const std::string medium_rotation_base =
        "{pos(60, 10), pos(60, 12), pos(62, 12), pos(62, 10), pos(60, 10)}";
    const std::string medium_rotated_full_cycle =
        nest(medium_rotation_base, "rotate_path({}, 1)", 360);
    const double medium_rotate_cycle_first_drift = parser.evaluate(
        "dist(" + medium_rotated_full_cycle + "[0], pos(60, 10))");
    const double medium_rotate_cycle_second_drift = parser.evaluate(
        "dist(" + medium_rotated_full_cycle + "[1], pos(60, 12))");
    const double medium_rotate_cycle_third_drift = parser.evaluate(
        "dist(" + medium_rotated_full_cycle + "[2], pos(62, 12))");
    const double medium_rotate_cycle_fourth_drift = parser.evaluate(
        "dist(" + medium_rotated_full_cycle + "[3], pos(62, 10))");
    const double medium_rotate_cycle_closed_drift = parser.evaluate(
        "dist(" + medium_rotated_full_cycle + "[4], pos(60, 10))");
    const std::string polar_base = "{pos(89, 0), pos(89, 50), pos(89.5, 25)}";
    const std::string polar_rotated_full_cycle = nest(polar_base, "rotate_path({}, 1, 1)", 360);
    const double polar_rotate_cycle_first_drift = parser.evaluate(
        "dist(" + polar_rotated_full_cycle + "[0], pos(89, 0))");
    const double polar_rotate_cycle_center_drift = parser.evaluate(
        "dist(" + polar_rotated_full_cycle + "[1], pos(89, 50))");
    const double polar_rotate_cycle_second_drift = parser.evaluate(
        "dist(" + polar_rotated_full_cycle + "[2], pos(89.5, 25))");
    const std::string polar_scaled_round_trip = nest(
        nest(polar_base, "scale_path({}, 0.5)", 10),
        "scale_path({}, 2)", 10);
    const double polar_scale_round_trip_first_drift = parser.evaluate(
        "dist(" + polar_scaled_round_trip + "[0], pos(89, 0))");
    const double polar_scale_round_trip_center_drift = parser.evaluate(
        "dist(" + polar_scaled_round_trip + "[1], pos(89, 50))");
    const double polar_scale_round_trip_second_drift = parser.evaluate(
        "dist(" + polar_scaled_round_trip + "[2], pos(89.5, 25))");
    const std::string polar_offset_round_trip = nest(
        nest(polar_base, "offset_path({}, 1000, 1000)", 10),
        "offset_path({}, -1000, -1000)", 10);
    const double polar_offset_round_trip_first_drift = parser.evaluate(
        "dist(" + polar_offset_round_trip + "[0], pos(89, 0))");
    const double polar_offset_round_trip_center_drift = parser.evaluate(
        "dist(" + polar_offset_round_trip + "[1], pos(89, 50))");
    const double polar_offset_round_trip_second_drift = parser.evaluate(
        "dist(" + polar_offset_round_trip + "[2], pos(89.5, 25))");
    if (!require(paired_values != nullptr && paired_values->size() == 2, "paired_values") ||
        !require(flattened_values != nullptr && flattened_values->size() == 4, "flattened_values") ||
        !require(empty_positions != nullptr && empty_positions->empty(), "empty_positions") ||
        !require(empty_scalars != nullptr && empty_scalars->empty(), "empty_scalars") ||
        !require(position_values != nullptr && position_values->size() == 2, "filled_positions") ||
        !require(densified_values != nullptr && densified_values->size() == 4, "densified_values") ||
        !require(offset_right_values != nullptr && offset_right_values->size() == 2, "offset_right_values") ||
        !require(offset_forward_values != nullptr && offset_forward_values->size() == 2, "offset_forward_values") ||
        !require(scaled_values != nullptr && scaled_values->size() == 3, "scaled_values") ||
        !require(simplified_values != nullptr && simplified_values->size() == 2, "simplified_values") ||
        !require(compressed_values != nullptr && compressed_values->size() == 2, "compressed_values") ||
        !require(scaled_closed_values != nullptr && scaled_closed_values->size() == 4, "scaled_closed_values") ||
        !require(zigzag_compressed_values != nullptr && zigzag_compressed_values->size() == 4,
                 "zigzag_compressed_values") ||
        !require(!densified_values || almost_equal(densified_values->front().longitude_deg, 0.0, 1e-12),
                 "densified_front") ||
        !require(!densified_values || almost_equal(densified_values->back().longitude_deg, 1.0, 1e-12),
                 "densified_back") ||
        !require(!offset_right_values ||
                     ((*offset_right_values)[0].latitude_deg > -0.0091 &&
                      (*offset_right_values)[0].latitude_deg < -0.0090),
                 "offset_right_first") ||
        !require(!offset_right_values ||
                     ((*offset_right_values)[1].latitude_deg > -0.0091 &&
                      (*offset_right_values)[1].latitude_deg < -0.0090),
                 "offset_right_second") ||
        !require(!offset_forward_values ||
                     ((*offset_forward_values)[0].longitude_deg > 0.0089 &&
                      (*offset_forward_values)[0].longitude_deg < 0.0091),
                 "offset_forward_first") ||
        !require(!offset_forward_values ||
                     ((*offset_forward_values)[1].longitude_deg > 1.0089 &&
                      (*offset_forward_values)[1].longitude_deg < 1.0091),
                 "offset_forward_second") ||
        !require(repeated_offset_middle_distance < 1e-3, "offset_repeat_middle_distance") ||
        !require(repeated_offset_path_length_delta < 1e-6, "offset_repeat_path_length_delta") ||
        !require(rotated_center_distance < 1e-9, "rotate_center_fixed") ||
        !require(rotated_first_radius_delta < 1e-6, "rotate_first_radius") ||
        !require(rotated_second_radius_delta < 1e-6, "rotate_second_radius") ||
        !require(repeated_rotation_middle_distance < 1e-6, "rotate_repeat_middle_distance") ||
        !require(scale_identity_delta < 1e-9, "scale_identity_delta") ||
        !require(scale_round_trip_first_drift < 0.5, "scale_round_trip_first_drift") ||
        !require(scale_round_trip_center_drift < 0.5, "scale_round_trip_center_drift") ||
        !require(scale_round_trip_second_drift < 0.5, "scale_round_trip_second_drift") ||
        !require(rotate_cycle_center_drift < 1e-6, "rotate_cycle_center_drift") ||
        !require(rotate_cycle_first_drift < 0.1, "rotate_cycle_first_drift") ||
        !require(rotate_cycle_second_drift < 0.1, "rotate_cycle_second_drift") ||
        !require(offset_round_trip_first_drift < 1e-6, "offset_round_trip_first_drift") ||
        !require(offset_round_trip_center_drift < 1e-6, "offset_round_trip_center_drift") ||
        !require(offset_round_trip_second_drift < 1e-6, "offset_round_trip_second_drift") ||
        !require(!simplified_values || almost_equal(simplified_values->front().longitude_deg, 0.0, 1e-12),
                 "simplified_front") ||
        !require(!simplified_values || almost_equal(simplified_values->back().longitude_deg, 1.0, 1e-12),
                 "simplified_back") ||
        !require(!compressed_values || almost_equal(compressed_values->front().longitude_deg, 0.0, 1e-12),
                 "compressed_front") ||
        !require(!compressed_values || almost_equal(compressed_values->back().longitude_deg, 1.0, 1e-12),
                 "compressed_back") ||
        !require(!scaled_closed_values ||
                     almost_equal(scaled_closed_values->front().latitude_deg,
                                  scaled_closed_values->back().latitude_deg, 1e-9),
                 "scaled_closed_lat") ||
        !require(!scaled_closed_values ||
                     almost_equal(scaled_closed_values->front().longitude_deg,
                                  scaled_closed_values->back().longitude_deg, 1e-9),
                 "scaled_closed_lon") ||
        !require(!zigzag_compressed_values ||
                     almost_equal((*zigzag_compressed_values)[0].latitude_deg, 60.0, 1e-9),
                 "zigzag_0_lat") ||
        !require(!zigzag_compressed_values ||
                     almost_equal((*zigzag_compressed_values)[0].longitude_deg, 10.0, 1e-9),
                 "zigzag_0_lon") ||
        !require(!zigzag_compressed_values ||
                     almost_equal((*zigzag_compressed_values)[1].latitude_deg, 61.0, 1e-9),
                 "zigzag_1_lat") ||
        !require(!zigzag_compressed_values ||
                     almost_equal((*zigzag_compressed_values)[1].longitude_deg, 11.0, 1e-9),
                 "zigzag_1_lon") ||
        !require(!zigzag_compressed_values ||
                     almost_equal((*zigzag_compressed_values)[2].latitude_deg, 62.0, 1e-9),
                 "zigzag_2_lat") ||
        !require(!zigzag_compressed_values ||
                     almost_equal((*zigzag_compressed_values)[2].longitude_deg, 10.0, 1e-9),
                 "zigzag_2_lon") ||
        !require(!zigzag_compressed_values ||
                     almost_equal((*zigzag_compressed_values)[3].latitude_deg, 63.0, 1e-9),
                 "zigzag_3_lat") ||
        !require(!zigzag_compressed_values ||
                     almost_equal((*zigzag_compressed_values)[3].longitude_deg, 11.0, 1e-9),
                 "zigzag_3_lon") ||
        !require(almost_equal(parser.evaluate("dist(densify_path({pos(0, 0), pos(0, 1)}, 2))"),
                              111319.4907932264, 1e-6),
                 "densified_dist") ||
        !require(almost_equal(
                     parser.evaluate("dist(offset_path({pos(0, 0), pos(0, 1)}, 1000, 0))"),
                     111319.4907932264, 5.0),
                 "offset_dist") ||
        !require(almost_equal(
                     parser.evaluate("dist(scale_path({pos(0, 0), pos(0, 1)}, 1))"),
                     111319.4907932264, 1e-6),
                 "scale_dist") ||
        !require(almost_equal(
                     parser.evaluate("dist(simplify_path(densify_path({pos(0, 0), pos(0, 1)}, 2), 1.0))"),
                     111319.4907932264, 1e-6),
                 "simplified_dist") ||
        !require(almost_equal(
                     parser.evaluate("dist(compress_path(densify_path({pos(0, 0), pos(0, 1)}, 4), 2))"),
                     111319.4907932264, 1e-6),
                 "compressed_dist") ||
        !require(indexed_position_value != nullptr, "indexed_position_type") ||
        !require(!indexed_position_value || almost_equal(indexed_position_value->latitude_deg, 61.0),
                 "indexed_position_lat") ||
        !require(!indexed_position_value || almost_equal(indexed_position_value->longitude_deg, 11.0),
                 "indexed_position_lon") ||
        !require(multi_position_distance_values != nullptr && multi_position_distance_values->size() == 2,
                 "multi_position_distance_values") ||
        !require(!multi_position_distance_values ||
                     almost_equal(scalar_to_double((*multi_position_distance_values)[0]), 111319.4907932264, 1e-6),
                 "multi_position_distance_0") ||
        !require(!multi_position_distance_values ||
                     almost_equal(scalar_to_double((*multi_position_distance_values)[1]), 111319.4907932264, 1e-6),
                 "multi_position_distance_1") ||
        !require(flattened_multi_position_values != nullptr && flattened_multi_position_values->size() == 2,
                 "flattened_multi_position_values") ||
        !require(!flattened_multi_position_values ||
                     (*flattened_multi_position_values)[0].size() == 4,
                 "flattened_multi_position_0_size") ||
        !require(!flattened_multi_position_values ||
                     almost_equal(scalar_to_double((*flattened_multi_position_values)[1][3]), 3.0),
                 "flattened_multi_position_1_3") ||
        !require(multi_densified_values != nullptr && multi_densified_values->size() == 2,
                 "multi_densified_values") ||
        !require(!multi_densified_values || (*multi_densified_values)[0].size() == 4,
                 "multi_densified_0_size") ||
        !require(!multi_densified_values || (*multi_densified_values)[1].size() == 4,
                 "multi_densified_1_size") ||
        !require(multi_offset_values != nullptr && multi_offset_values->size() == 2,
                 "multi_offset_values") ||
        !require(!multi_offset_values || (*multi_offset_values)[0].size() == 2,
                 "multi_offset_0_size") ||
        !require(!multi_offset_values || (*multi_offset_values)[1].size() == 2,
                 "multi_offset_1_size") ||
        !require(multi_rotated_values != nullptr && multi_rotated_values->size() == 2,
                 "multi_rotated_values") ||
        !require(!multi_rotated_values || (*multi_rotated_values)[0].size() == 3,
                 "multi_rotated_0_size") ||
        !require(!multi_rotated_values ||
                     almost_equal((*multi_rotated_values)[1][1].latitude_deg, 0.0, 1e-9),
                 "multi_rotated_center_lat") ||
        !require(!multi_rotated_values ||
                     almost_equal((*multi_rotated_values)[1][1].longitude_deg, 3.0, 1e-9),
                 "multi_rotated_center_lon") ||
        !require(multi_scaled_values != nullptr && multi_scaled_values->size() == 2,
                 "multi_scaled_values") ||
        !require(!multi_scaled_values || (*multi_scaled_values)[0].size() == 3,
                 "multi_scaled_0_size") ||
        !require(!multi_scaled_values ||
                     almost_equal((*multi_scaled_values)[1][1].latitude_deg, 0.0, 1e-9),
                 "multi_scaled_center_lat") ||
        !require(!multi_scaled_values ||
                     almost_equal((*multi_scaled_values)[1][1].longitude_deg, 3.0, 1e-9),
                 "multi_scaled_center_lon") ||
        !require(multi_simplified_values != nullptr && multi_simplified_values->size() == 2,
                 "multi_simplified_values") ||
        !require(!multi_simplified_values || (*multi_simplified_values)[0].size() == 2,
                 "multi_simplified_0_size") ||
        !require(!multi_simplified_values || (*multi_simplified_values)[1].size() == 2,
                 "multi_simplified_1_size") ||
        !require(multi_compressed_values != nullptr && multi_compressed_values->size() == 2,
                 "multi_compressed_values") ||
        !require(!multi_compressed_values || (*multi_compressed_values)[0].size() == 2,
                 "multi_compressed_0_size") ||
        !require(!multi_compressed_values || (*multi_compressed_values)[1].size() == 2,
                 "multi_compressed_1_size") ||
        !require(high_lat_segment_distance > 38000.0 && high_lat_segment_distance < 38200.0,
                 "high_lat_segment_distance") ||
        !require(high_lat_segment_bearing > 89.5 && high_lat_segment_bearing < 90.5,
                 "high_lat_segment_bearing") ||
        !require(almost_equal(high_lat_round_trip_lat, 70.0, 1e-8), "high_lat_round_trip_lat") ||
        !require(almost_equal(high_lat_round_trip_lon, 11.0, 1e-8), "high_lat_round_trip_lon") ||
        !require(high_lat_densified_values != nullptr && high_lat_densified_values->size() == 4,
                 "high_lat_densified_values") ||
        !require(!high_lat_densified_values ||
                     almost_equal(high_lat_densified_values->front().longitude_deg, 10.0, 1e-12),
                 "high_lat_densified_front") ||
        !require(!high_lat_densified_values ||
                     almost_equal(high_lat_densified_values->back().longitude_deg, 11.0, 1e-12),
                 "high_lat_densified_back") ||
        !require(high_lat_offset_right_values != nullptr && high_lat_offset_right_values->size() == 2,
                 "high_lat_offset_right_values") ||
        !require(!high_lat_offset_right_values ||
                     ((*high_lat_offset_right_values)[0].latitude_deg > 69.9910 &&
                      (*high_lat_offset_right_values)[0].latitude_deg < 69.9911),
                 "high_lat_offset_right_first") ||
        !require(!high_lat_offset_right_values ||
                     ((*high_lat_offset_right_values)[1].latitude_deg > 69.9910 &&
                      (*high_lat_offset_right_values)[1].latitude_deg < 69.9911),
                 "high_lat_offset_right_second") ||
        !require(high_lat_offset_forward_values != nullptr && high_lat_offset_forward_values->size() == 2,
                 "high_lat_offset_forward_values") ||
        !require(!high_lat_offset_forward_values ||
                     ((*high_lat_offset_forward_values)[0].longitude_deg > 10.0261 &&
                      (*high_lat_offset_forward_values)[0].longitude_deg < 10.0263),
                 "high_lat_offset_forward_first") ||
        !require(!high_lat_offset_forward_values ||
                     ((*high_lat_offset_forward_values)[1].longitude_deg > 11.0261 &&
                      (*high_lat_offset_forward_values)[1].longitude_deg < 11.0263),
                 "high_lat_offset_forward_second") ||
        !require(almost_equal(high_lat_offset_distance, high_lat_segment_distance, 5.0),
                 "high_lat_offset_distance") ||
        !require(high_lat_rotated_values != nullptr && high_lat_rotated_values->size() == 3,
                 "high_lat_rotated_values") ||
        !require(high_lat_rotated_center_distance < 1e-9, "high_lat_rotate_center_fixed") ||
        !require(high_lat_rotated_first_radius_delta < 1e-6, "high_lat_rotate_first_radius") ||
        !require(high_lat_rotated_second_radius_delta < 1e-6, "high_lat_rotate_second_radius") ||
        !require(high_lat_scaled_values != nullptr && high_lat_scaled_values->size() == 3,
                 "high_lat_scaled_values") ||
        !require(high_lat_scale_identity_delta < 1e-9, "high_lat_scale_identity_delta") ||
        !require(high_lat_scale_round_trip_first_drift < 0.5, "high_lat_scale_round_trip_first_drift") ||
        !require(high_lat_scale_round_trip_center_drift < 0.5, "high_lat_scale_round_trip_center_drift") ||
        !require(high_lat_scale_round_trip_second_drift < 0.5, "high_lat_scale_round_trip_second_drift") ||
        !require(high_lat_simplified_values != nullptr && high_lat_simplified_values->size() == 2,
                 "high_lat_simplified_values") ||
        !require(!high_lat_simplified_values ||
                     almost_equal(high_lat_simplified_values->front().longitude_deg, 10.0, 1e-12),
                 "high_lat_simplified_front") ||
        !require(!high_lat_simplified_values ||
                     almost_equal(high_lat_simplified_values->back().longitude_deg, 11.0, 1e-12),
                 "high_lat_simplified_back") ||
        !require(high_lat_compressed_values != nullptr && high_lat_compressed_values->size() == 2,
                 "high_lat_compressed_values") ||
        !require(!high_lat_compressed_values ||
                     almost_equal(high_lat_compressed_values->front().longitude_deg, 10.0, 1e-12),
                 "high_lat_compressed_front") ||
        !require(!high_lat_compressed_values ||
                     almost_equal(high_lat_compressed_values->back().longitude_deg, 11.0, 1e-12),
                 "high_lat_compressed_back") ||
        !require(medium_rotate_cycle_first_drift < 100.0, "medium_rotate_cycle_first_drift") ||
        !require(medium_rotate_cycle_second_drift < 100.0, "medium_rotate_cycle_second_drift") ||
        !require(medium_rotate_cycle_third_drift < 100.0, "medium_rotate_cycle_third_drift") ||
        !require(medium_rotate_cycle_fourth_drift < 100.0, "medium_rotate_cycle_fourth_drift") ||
        !require(medium_rotate_cycle_closed_drift < 100.0, "medium_rotate_cycle_closed_drift") ||
        !require(polar_rotate_cycle_first_drift < 1e-5, "polar_rotate_cycle_first_drift") ||
        !require(polar_rotate_cycle_center_drift < 1e-9, "polar_rotate_cycle_center_drift") ||
        !require(polar_rotate_cycle_second_drift < 1e-5, "polar_rotate_cycle_second_drift") ||
        !require(polar_scale_round_trip_first_drift < 1e-4, "polar_scale_round_trip_first_drift") ||
        !require(polar_scale_round_trip_center_drift < 1e-9, "polar_scale_round_trip_center_drift") ||
        !require(polar_scale_round_trip_second_drift < 1e-4, "polar_scale_round_trip_second_drift") ||
        !require(polar_offset_round_trip_first_drift < 500.0, "polar_offset_round_trip_first_drift") ||
        !require(polar_offset_round_trip_center_drift < 500.0, "polar_offset_round_trip_center_drift") ||
        !require(polar_offset_round_trip_second_drift < 500.0, "polar_offset_round_trip_second_drift")) {
        return false;
    }

    const Value integer_sum = parser.evaluate_value("1 + 2");
    const Value floating_division = parser.evaluate_value("1 / 2");
    const Value integer_sum_list = parser.evaluate_value("sum({1, 2, 3})");
    const Value mixed_sum_list = parser.evaluate_value("sum({1, 2.5})");
    const Value nested_sum_list = parser.evaluate_value("sum({{1, 2, 3}, {10, 20}})");
    const Value nested_product_list = parser.evaluate_value("product({{2, 3}, {4, 5}})");
    const Value nested_avg_list = parser.evaluate_value("avg({{1, 2}, {10, 20, 30}})");
    const Value nested_avg_avg = parser.evaluate_value("avg(avg({{1, 2}, {10, 20, 30}}))");
    const Value median_list = parser.evaluate_value("median({5, 1, 9})");
    const Value even_median_list = parser.evaluate_value("median({1, 2, 10, 20})");
    const Value nested_median_list = parser.evaluate_value("median({{5, 1, 9}, {1, 2, 10, 20}})");
    const Value nested_min_list = parser.evaluate_value("min({{2, -1, 5}, {10, 7}})");
    const Value nested_max_list = parser.evaluate_value("max({{2, -1, 5}, {10, 7}})");
    const Value nested_first_list = parser.evaluate_value("first({{1, 2, 3}, {10, 20}}, 1)");
    const Value nested_last_list = parser.evaluate_value("last({{1, 2, 3}, {10, 20}}, 1)");
    const Value default_first_list = parser.evaluate_value("first({9, 8, 7})");
    const Value default_last_list = parser.evaluate_value("last({9, 8, 7})");
    const Value nested_drop_list = parser.evaluate_value("drop({{1, 2, 3}, {10, 20}}, 1)");
    const Value sorted_list = parser.evaluate_value("sort({3, 1, 2})");
    const Value sorted_by_abs_list = parser.evaluate_value("sort_by({-3, 2, -1}, abs(_))");
    const Value reversed_list = parser.evaluate_value("reverse({3, 1, 2})");
    const Value reversed_multi_list = parser.evaluate_value("reverse({{1, 2}, {3, 4}})");
    const Value flattened_multi_list = parser.evaluate_value("flatten({{1, 2}, {3, 4}})");
    const Value flattened_multi_position_list = parser.evaluate_value(
        "flatten({{pos(60, 10), pos(61, 11)}, {pos(62, 12)}})");
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
        !std::holds_alternative<ListValue>(nested_sum_list) ||
        std::get<ListValue>(nested_sum_list).size() != 2 ||
        !almost_equal(scalar_to_double(std::get<ListValue>(nested_sum_list)[0]), 6.0) ||
        !almost_equal(scalar_to_double(std::get<ListValue>(nested_sum_list)[1]), 30.0) ||
        !std::holds_alternative<std::int64_t>(median_list) ||
        !almost_equal(scalar_to_double(std::get<std::int64_t>(median_list)), 5.0) ||
        !std::holds_alternative<double>(even_median_list) ||
        !almost_equal(std::get<double>(even_median_list), 6.0) ||
        !std::holds_alternative<ListValue>(nested_median_list) ||
        std::get<ListValue>(nested_median_list).size() != 2 ||
        !almost_equal(scalar_to_double(std::get<ListValue>(nested_median_list)[0]), 5.0) ||
        !almost_equal(scalar_to_double(std::get<ListValue>(nested_median_list)[1]), 6.0) ||
        !std::holds_alternative<ListValue>(nested_product_list) ||
        std::get<ListValue>(nested_product_list).size() != 2 ||
        !almost_equal(scalar_to_double(std::get<ListValue>(nested_product_list)[0]), 6.0) ||
        !almost_equal(scalar_to_double(std::get<ListValue>(nested_product_list)[1]), 20.0) ||
        !std::holds_alternative<ListValue>(nested_avg_list) ||
        std::get<ListValue>(nested_avg_list).size() != 2 ||
        !almost_equal(scalar_to_double(std::get<ListValue>(nested_avg_list)[0]), 1.5) ||
        !almost_equal(scalar_to_double(std::get<ListValue>(nested_avg_list)[1]), 20.0) ||
        !std::holds_alternative<double>(nested_avg_avg) ||
        !almost_equal(std::get<double>(nested_avg_avg), 10.75) ||
        !std::holds_alternative<ListValue>(nested_min_list) ||
        std::get<ListValue>(nested_min_list).size() != 2 ||
        !almost_equal(scalar_to_double(std::get<ListValue>(nested_min_list)[0]), -1.0) ||
        !almost_equal(scalar_to_double(std::get<ListValue>(nested_min_list)[1]), 7.0) ||
        !std::holds_alternative<ListValue>(nested_max_list) ||
        std::get<ListValue>(nested_max_list).size() != 2 ||
        !almost_equal(scalar_to_double(std::get<ListValue>(nested_max_list)[0]), 5.0) ||
        !almost_equal(scalar_to_double(std::get<ListValue>(nested_max_list)[1]), 10.0) ||
        !std::holds_alternative<MultiListValue>(nested_first_list) ||
        std::get<MultiListValue>(nested_first_list).size() != 1 ||
        std::get<MultiListValue>(nested_first_list)[0].size() != 3 ||
        !almost_equal(scalar_to_double(std::get<MultiListValue>(nested_first_list)[0][2]), 3.0) ||
        !std::holds_alternative<MultiListValue>(nested_last_list) ||
        std::get<MultiListValue>(nested_last_list).size() != 1 ||
        std::get<MultiListValue>(nested_last_list)[0].size() != 2 ||
        !almost_equal(scalar_to_double(std::get<MultiListValue>(nested_last_list)[0][1]), 20.0) ||
        !std::holds_alternative<ListValue>(default_first_list) ||
        std::get<ListValue>(default_first_list).size() != 1 ||
        !almost_equal(scalar_to_double(std::get<ListValue>(default_first_list)[0]), 9.0) ||
        !std::holds_alternative<ListValue>(default_last_list) ||
        std::get<ListValue>(default_last_list).size() != 1 ||
        !almost_equal(scalar_to_double(std::get<ListValue>(default_last_list)[0]), 7.0) ||
        !std::holds_alternative<MultiListValue>(nested_drop_list) ||
        std::get<MultiListValue>(nested_drop_list).size() != 2 ||
        std::get<MultiListValue>(nested_drop_list)[0].size() != 2 ||
        std::get<MultiListValue>(nested_drop_list)[1].size() != 1 ||
        !almost_equal(scalar_to_double(std::get<MultiListValue>(nested_drop_list)[0][0]), 2.0) ||
        !almost_equal(scalar_to_double(std::get<MultiListValue>(nested_drop_list)[1][0]), 20.0) ||
        !std::holds_alternative<ListValue>(sorted_list) ||
        std::get<ListValue>(sorted_list).size() != 3 ||
        !almost_equal(scalar_to_double(std::get<ListValue>(sorted_list)[0]), 1.0) ||
        !almost_equal(scalar_to_double(std::get<ListValue>(sorted_list)[2]), 3.0) ||
        !std::holds_alternative<ListValue>(sorted_by_abs_list) ||
        std::get<ListValue>(sorted_by_abs_list).size() != 3 ||
        !almost_equal(scalar_to_double(std::get<ListValue>(sorted_by_abs_list)[0]), -1.0) ||
        !almost_equal(scalar_to_double(std::get<ListValue>(sorted_by_abs_list)[1]), 2.0) ||
        !almost_equal(scalar_to_double(std::get<ListValue>(sorted_by_abs_list)[2]), -3.0) ||
        !std::holds_alternative<ListValue>(reversed_list) ||
        std::get<ListValue>(reversed_list).size() != 3 ||
        !almost_equal(scalar_to_double(std::get<ListValue>(reversed_list)[0]), 2.0) ||
        !almost_equal(scalar_to_double(std::get<ListValue>(reversed_list)[2]), 3.0) ||
        !std::holds_alternative<MultiListValue>(reversed_multi_list) ||
        std::get<MultiListValue>(reversed_multi_list).size() != 2 ||
        !almost_equal(scalar_to_double(std::get<MultiListValue>(reversed_multi_list)[0][0]), 3.0) ||
        !almost_equal(scalar_to_double(std::get<MultiListValue>(reversed_multi_list)[1][1]), 2.0) ||
        !std::holds_alternative<ListValue>(flattened_multi_list) ||
        std::get<ListValue>(flattened_multi_list).size() != 4 ||
        !almost_equal(scalar_to_double(std::get<ListValue>(flattened_multi_list)[2]), 3.0) ||
        !std::holds_alternative<PositionListValue>(flattened_multi_position_list) ||
        std::get<PositionListValue>(flattened_multi_position_list).size() != 3 ||
        !almost_equal(std::get<PositionListValue>(flattened_multi_position_list)[2].latitude_deg, 62.0) ||
        !almost_equal(std::get<PositionListValue>(flattened_multi_position_list)[2].longitude_deg, 12.0) ||
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
