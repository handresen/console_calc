#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct console_calc_binding_session console_calc_binding_session;

console_calc_binding_session* console_calc_binding_session_create(void);
void console_calc_binding_session_destroy(console_calc_binding_session* session);

int console_calc_binding_session_initialize(console_calc_binding_session* session);
int console_calc_binding_session_submit(console_calc_binding_session* session, const char* input);

const char* console_calc_binding_session_last_result_json(
    const console_calc_binding_session* session);

#ifdef __cplusplus
}
#endif
