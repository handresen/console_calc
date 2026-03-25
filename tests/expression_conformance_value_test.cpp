#include "expression_conformance_checks.h"

#include <exception>
#include <variant>

#include "expression_conformance_test_support.h"
#include "console_calc/value_utils.h"

namespace console_calc::test {

bool expect_expression_value_api(ExpressionParser& parser) {
    const Value scalar = parser.evaluate_value("1 + 2");
    if (!std::holds_alternative<std::int64_t>(scalar) ||
        value_kind(scalar) != ValueKind::scalar || !is_scalar_value(scalar) ||
        !almost_equal(scalar_to_double(std::get<std::int64_t>(scalar)), 3.0)) {
        return false;
    }

    const Value list = parser.evaluate_value("{1, 2, 3}");
    const auto* list_value = std::get_if<ListValue>(&list);
    if (list_value == nullptr || value_kind(list) != ValueKind::scalar_list ||
        !is_scalar_list_value(list) || !is_collection_value(list) || list_value->size() != 3) {
        return false;
    }

    if (!almost_equal(scalar_to_double((*list_value)[0]), 1.0) ||
        !almost_equal(scalar_to_double((*list_value)[1]), 2.0) ||
        !almost_equal(scalar_to_double((*list_value)[2]), 3.0)) {
        return false;
    }

    const Value position_list = parser.evaluate_value("{pos(60, 10), pos(61, 11)}");
    const auto* positions = std::get_if<PositionListValue>(&position_list);
    if (positions == nullptr || value_kind(position_list) != ValueKind::position_list ||
        !is_position_list_value(position_list) || !is_collection_value(position_list) ||
        positions->size() != 2 ||
        !almost_equal((*positions)[0].latitude_deg, 60.0) ||
        !almost_equal((*positions)[0].longitude_deg, 10.0) ||
        !almost_equal((*positions)[1].latitude_deg, 61.0) ||
        !almost_equal((*positions)[1].longitude_deg, 11.0)) {
        return false;
    }

    const Value position = parser.evaluate_value("pos(60, 10)");
    if (value_kind(position) != ValueKind::position || !is_position_value(position) ||
        is_collection_value(position)) {
        return false;
    }

    try {
        (void)parser.evaluate("{1, 2, 3}");
        return false;
    } catch (const std::invalid_argument&) {
    } catch (const std::exception&) {
        return false;
    }

    try {
        (void)parser.evaluate("sum(1)");
        return false;
    } catch (const std::invalid_argument&) {
    } catch (const std::exception&) {
        return false;
    }

    try {
        (void)parser.evaluate_value("sum({})");
    } catch (const std::exception&) {
        return false;
    }

    return true;
}

}  // namespace console_calc::test
