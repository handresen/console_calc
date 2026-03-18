#include "expression_case_loader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace console_calc::test {

namespace {

[[nodiscard]] std::string trim(const std::string& text) {
    const std::size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }

    const std::size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

}  // namespace

std::vector<ExpressionCase> load_expression_cases(const std::string& directory) {
    const std::string path = directory + "/expression_cases.txt";
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open expression test data file");
    }

    std::vector<ExpressionCase> cases;
    std::string line;
    while (std::getline(input, line)) {
        const std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        if (trimmed.size() < 3 || trimmed.front() != '"') {
            throw std::runtime_error("invalid test data line");
        }

        const std::size_t closing_quote = trimmed.find('"', 1);
        if (closing_quote == std::string::npos) {
            throw std::runtime_error("missing closing quote in test data");
        }

        const std::size_t comma = trimmed.find(',', closing_quote + 1);
        if (comma == std::string::npos) {
            throw std::runtime_error("missing comma in test data");
        }

        ExpressionCase expression_case;
        expression_case.expression = trimmed.substr(1, closing_quote - 1);

        const std::string expected = trim(trimmed.substr(comma + 1));
        if (expected == "INVALID") {
            expression_case.expect_invalid = true;
        } else {
            std::istringstream parser(expected);
            parser >> expression_case.expected_value;
            if (!parser || !parser.eof()) {
                throw std::runtime_error("invalid expected value in test data");
            }
        }

        cases.push_back(std::move(expression_case));
    }

    return cases;
}

}  // namespace console_calc::test
