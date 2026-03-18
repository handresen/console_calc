#include "console_history.h"

#include <utility>

namespace console_calc {

void ConsoleHistory::record(std::string command) {
    if (command.empty()) {
        return;
    }

    if (commands_.size() == k_max_history_size) {
        commands_.erase(commands_.begin());
    }

    commands_.push_back(std::move(command));
    reset_navigation();
}

void ConsoleHistory::reset_navigation() {
    navigation_index_.reset();
}

std::optional<std::string> ConsoleHistory::previous() {
    if (commands_.empty()) {
        return std::nullopt;
    }

    if (!navigation_index_.has_value()) {
        navigation_index_ = commands_.size() - 1;
        return commands_[*navigation_index_];
    }

    if (*navigation_index_ > 0) {
        --(*navigation_index_);
    }

    return commands_[*navigation_index_];
}

}  // namespace console_calc
