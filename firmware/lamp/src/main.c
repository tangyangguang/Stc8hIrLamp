#include "app_ir.h"
#include "app_light.h"
#include "board_init.h"
#include "board_pins.h"
#include "stc8h_interrupt.h"
#if APP_POWER_DOWN_ENABLE
#include "stc8h_power.h"
#endif
#include "stc8h_uart.h"

#if STC8H_SYSCLK_HZ == 6000000UL
#define APP_TICKS_PER_MS 500u
#elif STC8H_SYSCLK_HZ == 11059200UL
#define APP_TICKS_PER_MS 922u
#else
#define APP_TICKS_PER_MS ((stc8h_u16)(STC8H_SYSCLK_HZ / 12000UL))
#endif

#ifndef APP_SLEEP_GUARD_MS
#define APP_SLEEP_GUARD_MS 2000u
#endif

static STC8H_XDATA stc8h_u32 tick_accum;
static STC8H_XDATA stc8h_u16 sleep_guard_ms;
static STC8H_XDATA app_ir_event_t ir_event;

static void app_debug_write(const STC8H_CODE char *text)
{
#if APP_IR_UART_DEBUG && APP_IR_VERBOSE_DEBUG
    stc8h_uart_write_code(STC8H_UART1, text);
#else
    (void)text;
#endif
}

STC8H_INTERRUPT(app_int0_isr, STC8H_VECTOR_INT0)
{
    app_ir_on_edge_isr();
}

void main(void)
{
    stc8h_u16 last_ticks;
    stc8h_u16 now_ticks;

    board_init();
    app_ir_init();
    app_light_init();
    stc8h_interrupt_enable_global();
#if APP_IR_UART_DEBUG
    stc8h_uart_write_code(STC8H_UART1, "lamp boot\r\n");
#endif

    last_ticks = board_timer0_read();
    tick_accum = 0u;
    sleep_guard_ms = APP_SLEEP_GUARD_MS;

    while (1) {
        now_ticks = board_timer0_read();
        tick_accum += (stc8h_u16)(now_ticks - last_ticks);
        last_ticks = now_ticks;
        while (tick_accum >= APP_TICKS_PER_MS) {
            tick_accum -= APP_TICKS_PER_MS;
            app_light_tick_ms();
            if (sleep_guard_ms != 0u) {
                --sleep_guard_ms;
            }
        }

        app_ir_poll(&ir_event);
        if ((ir_event.command != APP_IR_COMMAND_NONE) || (app_ir_is_active() != 0u)) {
            sleep_guard_ms = APP_SLEEP_GUARD_MS;
        }
        app_light_handle_ir_event(&ir_event);

#if APP_POWER_DOWN_ENABLE
        if ((app_light_is_on() == 0u) &&
            (app_ir_is_active() == 0u) &&
            (sleep_guard_ms == 0u) &&
            (BOARD_IR_RX_READ() != 0u)) {
            app_debug_write("lamp sleep\r\n");
            board_prepare_power_down();
            app_ir_prepare_sleep();
            board_timer0_stop();
            stc8h_interrupt_enable_global();
            stc8h_power_down();
            board_prepare_active();
            board_timer0_start();
            board_debug_uart_init();
            app_ir_after_wake();
            app_debug_write("lamp wake\r\n");
            last_ticks = board_timer0_read();
            tick_accum = 0u;
            sleep_guard_ms = APP_SLEEP_GUARD_MS;
        }
#endif
    }
}
