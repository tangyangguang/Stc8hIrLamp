#include "board_init.h"
#include "board_pins.h"
#include "stc8h_exti.h"
#include "stc8h_gpio.h"
#include "stc8h_sfr.h"
#include "stc8h_timer.h"
#include "stc8h_uart.h"

#ifndef BOARD_LOW_POWER_UNUSED_IO
#define BOARD_LOW_POWER_UNUSED_IO 1
#endif

static void board_p1_input_only_no_pull(stc8h_u8 mask)
{
    P1M0 &= (stc8h_u8)~mask;
    P1M1 |= mask;
    P1PU &= (stc8h_u8)~mask;
    P1IE &= (stc8h_u8)~mask;
}

static void board_p3_input_only_no_pull(stc8h_u8 mask)
{
    P3M0 &= (stc8h_u8)~mask;
    P3M1 |= mask;
    P3PU &= (stc8h_u8)~mask;
    P3IE &= (stc8h_u8)~mask;
}

static void board_configure_unused_io_active(void)
{
#if BOARD_LOW_POWER_UNUSED_IO
    board_p1_input_only_no_pull(BOARD_P1_UNUSED_MASK);
#if APP_IR_UART_DEBUG
    board_p3_input_only_no_pull(BOARD_P3_ACTIVE_UNUSED_MASK);
#else
    board_p3_input_only_no_pull(BOARD_P3_POWER_UNUSED_MASK);
#endif
#endif
}

static void board_configure_unused_io_power_down(void)
{
#if BOARD_LOW_POWER_UNUSED_IO
    board_p1_input_only_no_pull(BOARD_P1_UNUSED_MASK);
    board_p3_input_only_no_pull(BOARD_P3_POWER_UNUSED_MASK);
#endif
}

void board_timer0_free_run_init(void)
{
    (void)stc8h_timer0_init_free_run_12t();
    stc8h_timer_start(STC8H_TIMER0);
}

void board_timer0_start(void)
{
    stc8h_timer_start(STC8H_TIMER0);
}

void board_timer0_stop(void)
{
    stc8h_timer_stop(STC8H_TIMER0);
}

void board_debug_uart_init(void)
{
#if APP_IR_UART_DEBUG
    (void)stc8h_uart_init(STC8H_UART1);
#endif
}

stc8h_u16 board_timer0_read(void)
{
    return stc8h_timer0_read();
}

stc8h_u16 board_timer0_ticks_to_us(stc8h_u16 ticks)
{
    return stc8h_timer0_12t_ticks_to_us(ticks);
}

void board_prepare_active(void)
{
    P_SW2 |= 0x80u;
    stc8h_gpio_set_mode(BOARD_LAMP_PWM_PORT, BOARD_LAMP_PWM_PIN, STC8H_GPIO_MODE_PUSH_PULL);
    stc8h_gpio_set_mode(BOARD_STATUS_LED_PORT, BOARD_STATUS_LED_PIN, STC8H_GPIO_MODE_PUSH_PULL);
    stc8h_gpio_set_mode(BOARD_IR_RX_PORT, BOARD_IR_RX_PIN, STC8H_GPIO_MODE_INPUT_ONLY);
    board_configure_unused_io_active();
    P3IE |= BOARD_IR_RX_MASK;
    P3PU |= BOARD_IR_RX_MASK;
}

void board_prepare_power_down(void)
{
    stc8h_gpio_write(BOARD_LAMP_PWM_PORT, BOARD_LAMP_PWM_PIN, 0u);
    stc8h_gpio_write(BOARD_STATUS_LED_PORT, BOARD_STATUS_LED_PIN, 0u);
    stc8h_gpio_set_mode(BOARD_IR_RX_PORT, BOARD_IR_RX_PIN, STC8H_GPIO_MODE_INPUT_ONLY);
    board_configure_unused_io_power_down();
    P3IE |= BOARD_IR_RX_MASK;
    P3PU |= BOARD_IR_RX_MASK;
}

void board_init(void)
{
    board_prepare_active();
    board_timer0_free_run_init();
    board_debug_uart_init();
    (void)stc8h_exti_configure(STC8H_EXTI_INT0, STC8H_EXTI_MODE_BOTH_EDGES);
    stc8h_exti_enable(STC8H_EXTI_INT0);
}
