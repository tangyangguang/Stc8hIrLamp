#include "app_ir.h"
#include "board_init.h"
#include "board_pins.h"
#include "drv_ir_rx.h"
#include "stc8h_exti.h"
#include "stc8h_interrupt.h"
#include "stc8h_sfr.h"
#include "stc8h_timer.h"
#include "stc8h_uart.h"

#define APP_IR_ADDRESS_REMOTE 0x00u
#define APP_IR_ADDRESS_LEGACY 0x01u
#define APP_IR_CODE_POWER 0x11u
#define APP_IR_CODE_BRIGHTER 0x22u
#define APP_IR_CODE_DIMMER 0x33u
#define APP_IR_CODE_TIMER_15_MIN 0x51u
#define APP_IR_CODE_TIMER_60_MIN 0x52u
#define APP_IR_IDLE_TIMEOUT_TICKS 27648u
#define APP_IR_LEVEL_MARK 0u
#define APP_IR_LEVEL_SPACE 1u

static STC8H_XDATA drv_ir_rx_t ir_rx;
static STC8H_XDATA drv_ir_rx_frame_t ir_frame;
static volatile stc8h_u8 edge_seen;
static volatile stc8h_u8 edge_level;
static volatile stc8h_u16 last_edge_ticks;
static volatile stc8h_u8 sleeping;
static volatile stc8h_u8 wake_seen;
#if APP_IR_UART_DEBUG && APP_IR_VERBOSE_DEBUG
static volatile STC8H_XDATA stc8h_u8 debug_interval_count;
static volatile STC8H_XDATA stc8h_u16 debug_first_interval_us;
static volatile STC8H_XDATA stc8h_u16 debug_last_interval_us;
static STC8H_XDATA stc8h_u8 debug_snapshot_count;
static STC8H_XDATA stc8h_u16 debug_snapshot_first_us;
static STC8H_XDATA stc8h_u16 debug_snapshot_last_us;
static STC8H_XDATA stc8h_u8 debug_snapshot_wake;
#endif
static app_ir_command_t last_command;

static app_ir_command_t app_ir_map_command(stc8h_u8 address, stc8h_u8 command)
{
    if (address == APP_IR_ADDRESS_REMOTE) {
        if (command == 0x18u) {
            return APP_IR_COMMAND_BRIGHTER;
        }
        if (command == 0x52u) {
            return APP_IR_COMMAND_DIMMER;
        }
    } else if (address == APP_IR_ADDRESS_LEGACY) {
        if (command == APP_IR_CODE_POWER) {
            return APP_IR_COMMAND_POWER;
        }
        if (command == APP_IR_CODE_BRIGHTER) {
            return APP_IR_COMMAND_BRIGHTER;
        }
        if (command == APP_IR_CODE_DIMMER) {
            return APP_IR_COMMAND_DIMMER;
        }
        if (command == APP_IR_CODE_TIMER_15_MIN) {
            return APP_IR_COMMAND_TIMER_15_MIN;
        }
        if (command == APP_IR_CODE_TIMER_60_MIN) {
            return APP_IR_COMMAND_TIMER_60_MIN;
        }
    }

    return APP_IR_COMMAND_NONE;
}

static stc8h_u8 app_ir_command_is_repeatable(app_ir_command_t command)
{
    return ((command == APP_IR_COMMAND_BRIGHTER) ||
            (command == APP_IR_COMMAND_DIMMER)) ? 1u : 0u;
}

#if APP_IR_UART_DEBUG
static void app_ir_debug_hex8(stc8h_u8 value)
{
    static const STC8H_CODE char hex[] = "0123456789ABCDEF";

    stc8h_uart_putc(STC8H_UART1, hex[value >> 4]);
    stc8h_uart_putc(STC8H_UART1, hex[value & 0x0Fu]);
}

static void app_ir_debug_hex16(stc8h_u16 value)
{
    app_ir_debug_hex8((stc8h_u8)(value >> 8));
    app_ir_debug_hex8((stc8h_u8)value);
}

static void app_ir_debug_write_action(app_ir_command_t command)
{
    if (command == APP_IR_COMMAND_POWER) {
        stc8h_uart_write_code(STC8H_UART1, "power");
    } else if (command == APP_IR_COMMAND_BRIGHTER) {
        stc8h_uart_write_code(STC8H_UART1, "brighter");
    } else if (command == APP_IR_COMMAND_DIMMER) {
        stc8h_uart_write_code(STC8H_UART1, "dimmer");
    } else if (command == APP_IR_COMMAND_TIMER_15_MIN) {
        stc8h_uart_write_code(STC8H_UART1, "timer15");
    } else if (command == APP_IR_COMMAND_TIMER_60_MIN) {
        stc8h_uart_write_code(STC8H_UART1, "timer60");
    } else {
        stc8h_uart_write_code(STC8H_UART1, "none");
    }
}

static void app_ir_debug_frame(stc8h_u8 address, stc8h_u8 command, app_ir_command_t action)
{
    stc8h_uart_write_code(STC8H_UART1, "ir frame addr=0x");
    app_ir_debug_hex8(address);
    stc8h_uart_write_code(STC8H_UART1, " cmd=0x");
    app_ir_debug_hex8(command);
    stc8h_uart_write_code(STC8H_UART1, " action=");
    app_ir_debug_write_action(action);
    stc8h_uart_write_code(STC8H_UART1, "\r\n");
}

static void app_ir_debug_repeat(app_ir_command_t action)
{
    stc8h_uart_write_code(STC8H_UART1, "ir repeat action=");
    app_ir_debug_write_action(action);
    stc8h_uart_write_code(STC8H_UART1, "\r\n");
}

static void app_ir_debug_none(stc8h_u8 count, stc8h_u16 first_us, stc8h_u16 last_us)
{
    stc8h_uart_write_code(STC8H_UART1, "ir none n=0x");
    app_ir_debug_hex8(count);
    stc8h_uart_write_code(STC8H_UART1, " first=0x");
    app_ir_debug_hex16(first_us);
    stc8h_uart_write_code(STC8H_UART1, " last=0x");
    app_ir_debug_hex16(last_us);
    stc8h_uart_write_code(STC8H_UART1, "\r\n");
}
#endif

void app_ir_init(void)
{
    drv_ir_rx_init(&ir_rx);
    edge_seen = 0u;
    edge_level = APP_IR_LEVEL_SPACE;
    last_edge_ticks = board_timer0_read();
    sleeping = 0u;
    wake_seen = 0u;
#if APP_IR_UART_DEBUG && APP_IR_VERBOSE_DEBUG
    debug_interval_count = 0u;
    debug_first_interval_us = 0u;
    debug_last_interval_us = 0u;
#endif
    last_command = APP_IR_COMMAND_NONE;
}

void app_ir_prepare_sleep(void)
{
    drv_ir_rx_reset(&ir_rx);
    sleeping = 1u;
    wake_seen = 0u;
    edge_seen = 0u;
    edge_level = APP_IR_LEVEL_SPACE;
    last_edge_ticks = board_timer0_read();
#if APP_IR_UART_DEBUG && APP_IR_VERBOSE_DEBUG
    debug_interval_count = 0u;
    debug_first_interval_us = 0u;
    debug_last_interval_us = 0u;
#endif
    stc8h_exti_clear_flag(STC8H_EXTI_INT0);
}

void app_ir_after_wake(void)
{
    sleeping = 0u;
}

void app_ir_poll(app_ir_event_t *result)
{
    drv_ir_rx_event_t event;
    app_ir_command_t command;
    stc8h_u16 now_ticks;
    stc8h_u16 edge_ticks;
    stc8h_u8 active;
    stc8h_u8 idle_expired;
#if APP_IR_UART_DEBUG && APP_IR_VERBOSE_DEBUG
    stc8h_u8 snapshot_count;
    stc8h_u16 snapshot_first_us;
    stc8h_u16 snapshot_last_us;
    stc8h_u8 snapshot_wake;
#endif

    if (result == 0) {
        return;
    }

    result->command = APP_IR_COMMAND_NONE;
    result->repeat = 0u;
    idle_expired = 0u;

    EA = 0;
    now_ticks = board_timer0_read();
    edge_ticks = last_edge_ticks;
    active = edge_seen;
#if APP_IR_UART_DEBUG && APP_IR_VERBOSE_DEBUG
    snapshot_wake = 0u;
    snapshot_count = 0u;
    snapshot_first_us = 0u;
    snapshot_last_us = 0u;
#endif
    EA = 1;

    if ((active != 0u) && ((stc8h_u16)(now_ticks - edge_ticks) > APP_IR_IDLE_TIMEOUT_TICKS)) {
        EA = 0;
        edge_seen = 0u;
        last_edge_ticks = now_ticks;
#if APP_IR_UART_DEBUG && APP_IR_VERBOSE_DEBUG
        snapshot_count = debug_interval_count;
        snapshot_first_us = debug_first_interval_us;
        snapshot_last_us = debug_last_interval_us;
        snapshot_wake = wake_seen;
        debug_interval_count = 0u;
        debug_first_interval_us = 0u;
        debug_last_interval_us = 0u;
#endif
        wake_seen = 0u;
        EA = 1;
        idle_expired = 1u;
    }

    EA = 0;
    event = drv_ir_rx_get_event(&ir_rx, &ir_frame);
    EA = 1;

    if (event == DRV_IR_RX_EVENT_FRAME) {
        command = app_ir_map_command(ir_frame.address, ir_frame.command);
#if APP_IR_UART_DEBUG
        app_ir_debug_frame(ir_frame.address, ir_frame.command, command);
#endif
        if (command != APP_IR_COMMAND_NONE) {
            last_command = app_ir_command_is_repeatable(command) ? command : APP_IR_COMMAND_NONE;
            result->command = command;
        }
    } else if (event == DRV_IR_RX_EVENT_REPEAT) {
        if (app_ir_command_is_repeatable(last_command) != 0u) {
#if APP_IR_UART_DEBUG
            app_ir_debug_repeat(last_command);
#endif
            result->command = last_command;
            result->repeat = 1u;
#if APP_IR_UART_DEBUG
        } else {
            app_ir_debug_repeat(APP_IR_COMMAND_NONE);
#endif
        }
    } else if (event == DRV_IR_RX_EVENT_NONE) {
#if APP_IR_UART_DEBUG && APP_IR_VERBOSE_DEBUG
        if ((snapshot_count != 0u) || (snapshot_wake != 0u)) {
            app_ir_debug_none(snapshot_count, snapshot_first_us, snapshot_last_us);
        }
#endif
    }

    if ((idle_expired != 0u) &&
        (event != DRV_IR_RX_EVENT_FRAME) &&
        (event != DRV_IR_RX_EVENT_REPEAT)) {
        EA = 0;
        drv_ir_rx_reset(&ir_rx);
        EA = 1;
    }
}

stc8h_u8 app_ir_is_active(void)
{
    stc8h_u16 now_ticks;
    stc8h_u16 edge_ticks;
    stc8h_u8 active;

    EA = 0;
    now_ticks = board_timer0_read();
    edge_ticks = last_edge_ticks;
    active = edge_seen;
    EA = 1;

    if (active == 0u) {
        return 0u;
    }
    return ((stc8h_u16)(now_ticks - edge_ticks) <= APP_IR_IDLE_TIMEOUT_TICKS) ? 1u : 0u;
}

void app_ir_on_edge_isr(void)
{
    stc8h_u16 now_ticks;
    stc8h_u16 width_ticks;
    stc8h_u16 width_us;
    stc8h_u8 current_level;

    now_ticks = board_timer0_read();
    current_level = (BOARD_IR_RX_READ() != 0u) ? APP_IR_LEVEL_SPACE : APP_IR_LEVEL_MARK;

    if (sleeping != 0u) {
        sleeping = 0u;
        stc8h_timer0_reset();
        stc8h_timer_start(STC8H_TIMER0);
        edge_seen = 1u;
        edge_level = current_level;
        last_edge_ticks = board_timer0_read();
        wake_seen = 1u;
        drv_ir_rx_reset(&ir_rx);
        return;
    }

    if (edge_seen != 0u) {
        width_ticks = (stc8h_u16)(now_ticks - last_edge_ticks);
        width_us = board_timer0_ticks_to_us(width_ticks);
#if APP_IR_UART_DEBUG && APP_IR_VERBOSE_DEBUG
        if (debug_interval_count == 0u) {
            debug_first_interval_us = width_us;
        }
        if (debug_interval_count != 0xFFu) {
            ++debug_interval_count;
        }
        debug_last_interval_us = width_us;
#endif
        drv_ir_rx_feed_pulse(&ir_rx, edge_level, width_us);
    }
    edge_seen = 1u;
    edge_level = current_level;
    last_edge_ticks = now_ticks;
}
