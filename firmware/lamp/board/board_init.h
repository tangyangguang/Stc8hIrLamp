#ifndef BOARD_INIT_H
#define BOARD_INIT_H

#include "stc8h_config.h"

void board_init(void);
void board_debug_uart_init(void);
void board_prepare_active(void);
void board_prepare_power_down(void);
void board_timer0_free_run_init(void);
void board_timer0_start(void);
void board_timer0_stop(void);
stc8h_u16 board_timer0_read(void);
stc8h_u16 board_timer0_ticks_to_us(stc8h_u16 ticks);

#endif
