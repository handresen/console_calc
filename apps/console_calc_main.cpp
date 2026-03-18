#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#include "console_calc_app.h"
#include "currency_rate_provider.h"

int main(int argc, char* argv[]) {
    auto currency_rate_provider = console_calc::make_default_currency_rate_provider();
    std::vector<std::string_view> args;
    args.reserve(argc > 0 ? static_cast<std::size_t>(argc - 1) : 0U);
    for (int index = 1; index < argc; ++index) {
        args.emplace_back(argv[index]);
    }

    return console_calc::run_console_calc(
        args, std::cin, std::cout, std::cerr,
        console_calc::ConsoleCalcOptions{
            .currency_rate_provider = currency_rate_provider.get(),
            .auto_refresh_currency_rates = true,
            .currency_rate_timeout = std::chrono::milliseconds{1500},
        });
}
