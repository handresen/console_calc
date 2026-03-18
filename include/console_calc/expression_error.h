#pragma once

#include <stdexcept>
#include <string>

namespace console_calc {

class ParseError : public std::invalid_argument {
public:
    explicit ParseError(const std::string& message) : std::invalid_argument(message) {}
};

class EvaluationError : public std::invalid_argument {
public:
    explicit EvaluationError(const std::string& message) : std::invalid_argument(message) {}
};

}  // namespace console_calc
