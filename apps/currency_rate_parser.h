#pragma once

#include <span>
#include <string_view>

#include "currency_rate_provider.h"

namespace console_calc {

[[nodiscard]] CurrencyFetchResult parse_nok_currency_rates_response(
    std::string_view response_body, std::span<const std::string_view> currencies);

}  // namespace console_calc
