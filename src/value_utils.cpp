#include "console_calc/value_utils.h"

#include <variant>

namespace console_calc {

ValueKind value_kind(const Value& value) {
    if (std::holds_alternative<std::int64_t>(value) || std::holds_alternative<double>(value)) {
        return ValueKind::scalar;
    }
    if (std::holds_alternative<ListValue>(value)) {
        return ValueKind::scalar_list;
    }
    if (std::holds_alternative<MultiListValue>(value)) {
        return ValueKind::multi_scalar_list;
    }
    if (std::holds_alternative<PositionValue>(value)) {
        return ValueKind::position;
    }
    if (std::holds_alternative<PositionListValue>(value)) {
        return ValueKind::position_list;
    }
    return ValueKind::multi_position_list;
}

std::string_view value_kind_name(ValueKind kind) {
    switch (kind) {
    case ValueKind::scalar:
        return "scalar";
    case ValueKind::scalar_list:
        return "scalar list";
    case ValueKind::multi_scalar_list:
        return "multi scalar list";
    case ValueKind::position:
        return "position";
    case ValueKind::position_list:
        return "position list";
    case ValueKind::multi_position_list:
        return "multi position list";
    }
    return "unknown";
}

bool is_scalar_value(const Value& value) {
    return value_kind(value) == ValueKind::scalar;
}

bool is_scalar_list_value(const Value& value) {
    return value_kind(value) == ValueKind::scalar_list;
}

bool is_multi_scalar_list_value(const Value& value) {
    return value_kind(value) == ValueKind::multi_scalar_list;
}

bool is_position_value(const Value& value) {
    return value_kind(value) == ValueKind::position;
}

bool is_position_list_value(const Value& value) {
    return value_kind(value) == ValueKind::position_list;
}

bool is_multi_position_list_value(const Value& value) {
    return value_kind(value) == ValueKind::multi_position_list;
}

bool is_collection_value(const Value& value) {
    const ValueKind kind = value_kind(value);
    return kind == ValueKind::scalar_list || kind == ValueKind::multi_scalar_list ||
           kind == ValueKind::position_list || kind == ValueKind::multi_position_list;
}

}  // namespace console_calc
