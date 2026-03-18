#pragma once

#include <array>
#include <string_view>

namespace console_calc {

struct CurrencyCatalogEntry {
    std::string_view upper_code;
    std::string_view lower_code;
};

constexpr std::array<CurrencyCatalogEntry, 6> k_console_currency_catalog{{
    {"USD", "usd"},
    {"CNY", "cny"},
    {"EUR", "eur"},
    {"GBP", "gbp"},
    {"SEK", "sek"},
    {"DKK", "dkk"},
}};

}  // namespace console_calc
