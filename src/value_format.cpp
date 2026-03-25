#include "console_calc/value_format.h"
#include "console_calc/value_utils.h"

#include <cstdint>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>

namespace console_calc {

std::string format_scalar(const ScalarValue& value) {
    return format_scalar(value, IntegerDisplayMode::decimal);
}

std::string format_scalar(const ScalarValue& value, IntegerDisplayMode mode) {
    if (const auto* integer = std::get_if<std::int64_t>(&value)) {
        if (mode == IntegerDisplayMode::decimal) {
            return std::to_string(*integer);
        }

        const bool negative = *integer < 0;
        std::uint64_t magnitude = negative ? static_cast<std::uint64_t>(-(*integer + 1)) + 1U
                                           : static_cast<std::uint64_t>(*integer);
        std::string digits;
        if (mode == IntegerDisplayMode::hexadecimal) {
            static constexpr char k_hex_digits[] = "0123456789abcdef";
            do {
                digits.push_back(k_hex_digits[magnitude & 0xfU]);
                magnitude >>= 4U;
            } while (magnitude != 0);
            std::reverse(digits.begin(), digits.end());
            return std::string(negative ? "-0x" : "0x") + digits;
        }

        do {
            digits.push_back((magnitude & 1U) != 0 ? '1' : '0');
            magnitude >>= 1U;
        } while (magnitude != 0);
        std::reverse(digits.begin(), digits.end());
        return std::string(negative ? "-0b" : "0b") + digits;
    }

    std::ostringstream stream;
    stream << std::setprecision(std::numeric_limits<double>::max_digits10)
           << std::get<double>(value);
    return stream.str();
}

std::string format_position(const PositionValue& value) {
    return "pos(" + format_scalar(ScalarValue{value.latitude_deg}) + ", " +
           format_scalar(ScalarValue{value.longitude_deg}) + ")";
}

std::string format_list(const ListValue& values) {
    return format_list(values, IntegerDisplayMode::decimal);
}

std::string format_list(const ListValue& values, IntegerDisplayMode mode) {
    std::string result = "{";
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            result += ", ";
        }
        result += format_scalar(values[index], mode);
    }
    result += '}';
    return result;
}

std::string format_multi_list(const MultiListValue& values) {
    return format_multi_list(values, IntegerDisplayMode::decimal);
}

std::string format_multi_list(const MultiListValue& values, IntegerDisplayMode mode) {
    std::string result = "{";
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            result += ", ";
        }
        result += format_list(values[index], mode);
    }
    result += '}';
    return result;
}

std::string format_position_list(const PositionListValue& values) {
    std::string result = "{";
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            result += ", ";
        }
        result += format_position(values[index]);
    }
    result += '}';
    return result;
}

std::string format_value(const Value& value) {
    return format_value(value, IntegerDisplayMode::decimal);
}

std::string format_value(const Value& value, IntegerDisplayMode mode) {
    switch (value_kind(value)) {
    case ValueKind::scalar:
        if (const auto* integer = std::get_if<std::int64_t>(&value)) {
            return format_scalar(ScalarValue{*integer}, mode);
        }
        return format_scalar(ScalarValue{std::get<double>(value)}, mode);
    case ValueKind::position:
        return format_position(std::get<PositionValue>(value));
    case ValueKind::scalar_list:
        return format_list(std::get<ListValue>(value), mode);
    case ValueKind::multi_scalar_list:
        return format_multi_list(std::get<MultiListValue>(value), mode);
    case ValueKind::position_list:
        return format_position_list(std::get<PositionListValue>(value));
    }

    return {};
}

}  // namespace console_calc
