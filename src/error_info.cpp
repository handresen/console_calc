#include "console_calc/error_info.h"

#include "console_calc/builtin_function.h"
#include "console_calc/special_form.h"

namespace console_calc {

ErrorInfo infer_error_info(std::string_view message) {
    ErrorInfo error{.message = std::string(message)};

    constexpr std::string_view prefix = "function '";
    if (!message.starts_with(prefix)) {
        return error;
    }

    const std::size_t name_begin = prefix.size();
    const std::size_t name_end = message.find('\'', name_begin);
    if (name_end == std::string_view::npos || name_end == name_begin) {
        return error;
    }

    const std::string_view name = message.substr(name_begin, name_end - name_begin);
    const auto function = parse_builtin_function(name);
    if (function.has_value()) {
        error.expected_signature = std::string(builtin_function_signature(*function));
        return error;
    }

    const auto special_form = parse_special_form_function(name);
    if (!special_form.has_value()) {
        return error;
    }

    error.expected_signature = std::string(special_form_signature(*special_form));
    return error;
}

}  // namespace console_calc
