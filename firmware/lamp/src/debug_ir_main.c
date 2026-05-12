#include "board_init.h"
#include "board_pins.h"
#include "drv_ir_rx.h"
#include "stc8h_exti.h"
#include "stc8h_interrupt.h"
#include "stc8h_timer.h"
#include "stc8h_uart.h"

#define DEBUG_IDLE_TIMEOUT_TICKS 27648u
#define DEBUG_INTERVAL_MAX 48u

static STC8H_XDATA drv_ir_rx_t ir_rx;
static volatile STC8H_XDATA stc8h_u16 intervals_us[DEBUG_INTERVAL_MAX];
static volatile stc8h_u8 interval_count;
static volatile stc8h_u8 interval_overflow;
static volatile stc8h_u8 falling_seen;
static volatile stc8h_u16 last_falling_ticks;

static void uart_hex8(stc8h_u8 value)
{
    static const STC8H_CODE char hex[] = "0123456789ABCDEF";

    stc8h_uart_putc(STC8H_UART1, hex[value >> 4]);
    stc8h_uart_putc(STC8H_UART1, hex[value & 0x0Fu]);
}

static void uart_u16(stc8h_u16 value)
{
    static const STC8H_CODE stc8h_u16 divisors[] = {
        10000u, 1000u, 100u, 10u, 1u
    };
    stc8h_u8 i;
    stc8h_u8 started;
    stc8h_u8 digit;

    started = 0u;
    for (i = 0u; i < 5u; ++i) {
        digit = 0u;
        while (value >= divisors[i]) {
            value = (stc8h_u16)(value - divisors[i]);
            ++digit;
        }
        if ((digit != 0u) || (started != 0u) || (i == 4u)) {
            started = 1u;
            stc8h_uart_putc(STC8H_UART1, (char)('0' + digit));
        }
    }
}

static void print_decode_event(void)
{
    drv_ir_rx_frame_t frame;
    drv_ir_rx_event_t event;

    event = drv_ir_rx_get_event(&ir_rx, &frame);
    if (event == DRV_IR_RX_EVENT_FRAME) {
        stc8h_uart_write_code(STC8H_UART1, "decode frame addr=0x");
        uart_hex8(frame.address);
        stc8h_uart_write_code(STC8H_UART1, " cmd=0x");
        uart_hex8(frame.command);
        stc8h_uart_write_code(STC8H_UART1, "\r\n");
    } else if (event == DRV_IR_RX_EVENT_REPEAT) {
        stc8h_uart_write_code(STC8H_UART1, "decode repeat\r\n");
    } else if (event == DRV_IR_RX_EVENT_ERROR) {
        stc8h_uart_write_code(STC8H_UART1, "decode error\r\n");
    } else {
        stc8h_uart_write_code(STC8H_UART1, "decode none\r\n");
    }
}

static void print_frame(void)
{
    stc8h_u8 i;
    stc8h_u8 count;
    stc8h_u8 overflow;

    stc8h_exti_disable(STC8H_EXTI_INT0);
    count = interval_count;
    overflow = interval_overflow;

    stc8h_uart_write_code(STC8H_UART1, "falling count=");
    uart_u16(count);
    stc8h_uart_write_code(STC8H_UART1, " overflow=");
    stc8h_uart_putc(STC8H_UART1, overflow ? '1' : '0');
    stc8h_uart_write_code(STC8H_UART1, "\r\n");

    drv_ir_rx_reset(&ir_rx);
    for (i = 0u; i < count; ++i) {
        uart_u16(i);
        stc8h_uart_write_code(STC8H_UART1, ":");
        uart_u16(intervals_us[i]);
        stc8h_uart_write_code(STC8H_UART1, "us\r\n");
        drv_ir_rx_feed_nec_falling_interval(&ir_rx, intervals_us[i]);
    }
    drv_ir_rx_finish_nec_falling_interval(&ir_rx);
    print_decode_event();

    interval_count = 0u;
    interval_overflow = 0u;
    falling_seen = 0u;
    last_falling_ticks = stc8h_timer0_read();
    stc8h_exti_clear_flag(STC8H_EXTI_INT0);
    stc8h_exti_enable(STC8H_EXTI_INT0);
}

STC8H_INTERRUPT(debug_ir_int0_isr, STC8H_VECTOR_INT0)
{
    stc8h_u16 now_ticks;
    stc8h_u16 width_ticks;
    stc8h_u8 index;

    now_ticks = stc8h_timer0_read();

    if (falling_seen != 0u) {
        width_ticks = (stc8h_u16)(now_ticks - last_falling_ticks);
        index = interval_count;
        if (index < DEBUG_INTERVAL_MAX) {
            intervals_us[index] = stc8h_timer0_12t_ticks_to_us(width_ticks);
            interval_count = (stc8h_u8)(index + 1u);
        } else {
            interval_overflow = 1u;
        }
    }

    falling_seen = 1u;
    last_falling_ticks = now_ticks;
}

void main(void)
{
    stc8h_u16 now_ticks;
    stc8h_u16 falling_ticks;
    stc8h_u16 idle_ticks;
    stc8h_u8 count;

    board_prepare_active();
    board_timer0_free_run_init();
    (void)stc8h_exti_configure(STC8H_EXTI_INT0, STC8H_EXTI_MODE_FALLING_EDGE);
    stc8h_exti_enable(STC8H_EXTI_INT0);
    (void)stc8h_uart_init(STC8H_UART1);

    drv_ir_rx_init(&ir_rx);
    interval_count = 0u;
    interval_overflow = 0u;
    falling_seen = 0u;
    last_falling_ticks = stc8h_timer0_read();

    stc8h_interrupt_enable_global();

    stc8h_uart_write_code(STC8H_UART1, "IR falling debug boot 6000000Hz UART1 115200 P32\r\n");
    stc8h_uart_write_code(STC8H_UART1, "idle level=");
    stc8h_uart_putc(STC8H_UART1, BOARD_IR_RX_READ() ? '1' : '0');
    stc8h_uart_write_code(STC8H_UART1, "\r\n");

    while (1) {
        EA = 0;
        now_ticks = stc8h_timer0_read();
        falling_ticks = last_falling_ticks;
        count = interval_count;
        EA = 1;
        idle_ticks = (stc8h_u16)(now_ticks - falling_ticks);
        if ((count != 0u) && (idle_ticks > DEBUG_IDLE_TIMEOUT_TICKS)) {
            print_frame();
        }
    }
}
