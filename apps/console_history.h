#pragma once

#include <optional>
#include <string>
#include <vector>

namespace console_calc {

class ConsoleHistory {
public:
    void record(std::string command);
    void reset_navigation();
    [[nodiscard]] std::optional<std::string> previous();

private:
    static constexpr std::size_t k_max_history_size = 10;

    std::vector<std::string> commands_;
    std::optional<std::size_t> navigation_index_;
};

}  // namespace console_calc
