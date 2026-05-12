#ifndef APP_IR_H
#define APP_IR_H

#include "stc8h_config.h"

typedef enum {
    APP_IR_COMMAND_NONE = 0,
    APP_IR_COMMAND_POWER,
    APP_IR_COMMAND_BRIGHTER,
    APP_IR_COMMAND_DIMMER,
    APP_IR_COMMAND_TIMER_15_MIN,
    APP_IR_COMMAND_TIMER_60_MIN
} app_ir_command_t;

typedef struct {
    app_ir_command_t command;
    stc8h_u8 repeat;
} app_ir_event_t;

void app_ir_init(void);
void app_ir_prepare_sleep(void);
void app_ir_after_wake(void);
void app_ir_poll(app_ir_event_t *event);
stc8h_u8 app_ir_is_active(void);
void app_ir_on_edge_isr(void);

#endif
