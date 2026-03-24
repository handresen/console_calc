#include "currency_definition_materializer.h"

#include <string>

#include "currency_catalog.h"
#include "console_calc/value_format.h"

namespace console_calc {

void apply_currency_rate_definitions(DefinitionTable& definitions,
                                     const CurrencyRateTable& rates) {
    for (const auto& entry : k_console_currency_catalog) {
        const auto found = rates.find(std::string(entry.lower_code));
        if (found == rates.end()) {
            continue;
        }

        const std::string direct_name = "nok2" + std::string(entry.lower_code);
        const std::string inverse_name = std::string(entry.lower_code) + "2nok";
        definitions[direct_name] = make_value_definition(format_scalar(found->second));
        definitions[inverse_name] = make_value_definition(format_scalar(1.0 / found->second));
    }
}

}  // namespace console_calc
