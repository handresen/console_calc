#include "console_calc/value_format.h"

#include <iomanip>
#include <limits>
#include <sstream>

namespace console_calc {

std::string format_scalar(double value) {
    std::ostringstream stream;
    stream << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
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
    if (const auto* scalar = std::get_if<double>(&value)) {
        return format_scalar(*scalar);
    }

    return format_list(std::get<ListValue>(value));
}

}  // namespace console_calc
