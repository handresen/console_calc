#include "console_calc/value_format.h"

#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>

namespace console_calc {

std::string format_scalar(const ScalarValue& value) {
    if (const auto* integer = std::get_if<std::int64_t>(&value)) {
        return std::to_string(*integer);
    }

    std::ostringstream stream;
    stream << std::setprecision(std::numeric_limits<double>::max_digits10)
           << std::get<double>(value);
    return stream.str();
}

std::string format_list(const ListValue& values) {
    std::string result = "{";
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            result += ", ";
        }
        result += format_scalar(values[index]);
    }
    result += '}';
    return result;
}

std::string format_value(const Value& value) {
    if (const auto* integer = std::get_if<std::int64_t>(&value)) {
        return format_scalar(ScalarValue{*integer});
    }

    if (const auto* scalar = std::get_if<double>(&value)) {
        return format_scalar(ScalarValue{*scalar});
    }

    return format_list(std::get<ListValue>(value));
}

}  // namespace console_calc
