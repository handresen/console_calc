#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace console_calc {

struct ErrorInfo {
    std::string message;
    std::optional<std::string> expected_signature;
};

[[nodiscard]] ErrorInfo infer_error_info(std::string_view message);

}  // namespace console_calc
