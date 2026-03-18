#include <iostream>
#include <string_view>
#include <vector>

#include "console_calc_app.h"

int main(int argc, char* argv[]) {
    std::vector<std::string_view> args;
    args.reserve(argc > 0 ? static_cast<std::size_t>(argc - 1) : 0U);
    for (int index = 1; index < argc; ++index) {
        args.emplace_back(argv[index]);
    }

    return console_calc::run_console_calc(args, std::cin, std::cout, std::cerr);
}
