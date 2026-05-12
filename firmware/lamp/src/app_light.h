#ifndef APP_LIGHT_H
#define APP_LIGHT_H

#include "app_ir.h"
#include "stc8h_config.h"

void app_light_init(void);
void app_light_tick_ms(void);
void app_light_handle_ir_event(const app_ir_event_t *event);
stc8h_u8 app_light_is_on(void);

#endif
