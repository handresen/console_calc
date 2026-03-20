#include <cstdlib>

#include "console_session_test_support.h"

int main() {
    if (!console_calc::test::expect_console_mode_basic_behaviors() ||
        !console_calc::test::expect_console_mode_definition_behaviors() ||
        !console_calc::test::expect_console_mode_list_behaviors() ||
        !console_calc::test::expect_console_mode_listing_and_currency_behaviors()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
