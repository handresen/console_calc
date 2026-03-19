#pragma once

#include "currency_rate_provider.h"
#include "expression_environment.h"

namespace console_calc {

void apply_currency_rate_definitions(DefinitionTable& definitions,
                                     const CurrencyRateTable& rates);

}  // namespace console_calc
