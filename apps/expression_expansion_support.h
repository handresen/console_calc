#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace console_calc::detail {

enum class ExpansionFrameKind {
    group,
    list,
    call,
};

struct ExpansionFrame {
    ExpansionFrameKind kind = ExpansionFrameKind::group;
    std::string identifier;
    std::size_t argument_index = 0;
};

[[nodiscard]] bool is_identifier_start(char ch);
[[nodiscard]] bool is_identifier_char(char ch);
[[nodiscard]] std::size_t skip_whitespace(std::string_view expression, std::size_t index);
[[nodiscard]] bool is_blank_text(std::string_view text);
[[nodiscard]] bool is_followed_by_call(std::string_view expression, std::size_t index);
[[nodiscard]] std::size_t consume_radix_literal(std::string_view expression, std::size_t index);
[[nodiscard]] bool is_inside_placeholder_expression(const std::vector<ExpansionFrame>& frames);
[[nodiscard]] bool is_builtin_or_special_call(std::string_view identifier, bool followed_by_call);
[[nodiscard]] std::size_t call_open_paren_index(std::string_view expression, std::size_t index);
[[nodiscard]] std::size_t find_call_close_paren(std::string_view expression,
                                                std::size_t open_paren_index);
[[nodiscard]] std::vector<std::string> extract_call_arguments(
    std::string_view expression, std::size_t open_paren_index,
    std::size_t close_paren_index);
[[nodiscard]] std::string substitute_function_parameters(
    std::string_view expression, std::span<const std::string> parameter_names,
    std::span<const std::string> replacement_expressions);

}  // namespace console_calc::detail
