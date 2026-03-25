#pragma once

#include <string_view>

#include "console_calc/value.h"

namespace console_calc {

enum class ValueKind {
    scalar,
    scalar_list,
    multi_scalar_list,
    position,
    position_list,
};

[[nodiscard]] ValueKind value_kind(const Value& value);
[[nodiscard]] std::string_view value_kind_name(ValueKind kind);
[[nodiscard]] bool is_scalar_value(const Value& value);
[[nodiscard]] bool is_scalar_list_value(const Value& value);
[[nodiscard]] bool is_multi_scalar_list_value(const Value& value);
[[nodiscard]] bool is_position_value(const Value& value);
[[nodiscard]] bool is_position_list_value(const Value& value);
[[nodiscard]] bool is_collection_value(const Value& value);

}  // namespace console_calc
