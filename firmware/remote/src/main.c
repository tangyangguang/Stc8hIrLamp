#define STC8H_CONFIG_INCLUDE "board_config.h"
#define STC8H_PINS_INCLUDE "board_pins.h"

#include "app_config.h"
#include "board_pins.h"
#include "drv_ir_tx.h"
#include "stc8h_delay.h"
#include "stc8h_exti.h"
#include "stc8h_gpio.h"
#include "stc8h_interrupt.h"
#include "stc8h_power.h"
#include "stc8h_pwm.h"

#define APP_KEY_NONE  0u
#define APP_KEY_POWER 1u
#define APP_KEY_UP    2u
#define APP_KEY_DOWN  3u
#define APP_KEY_FN1   4u
#define APP_KEY_FN2   5u

#define APP_POLL_STEP_MS 10u

static volatile stc8h_u8 app_woke;
static stc8h_u8 app_sleep_while_key_pressed;

static void app_delay_ms(stc8h_u16 ms)
{
    while (ms >= 10u) {
        stc8h_delay_timer0_1t_us(10000u);
        ms = (stc8h_u16)(ms - 10u);
    }
    while (ms != 0u) {
        stc8h_delay_timer0_1t_us(1000u);
        --ms;
    }
}

static void app_led_on(void)
{
#if APP_LED_FEEDBACK_ENABLE
    stc8h_gpio_write(BOARD_LED_PORT, BOARD_LED_PIN, 1u);
#endif
}

static void app_led_off(void)
{
    stc8h_gpio_write(BOARD_LED_PORT, BOARD_LED_PIN, 0u);
}

static void app_led_flash_once(void)
{
#if APP_LED_FEEDBACK_ENABLE
    app_led_on();
    app_delay_ms(APP_LED_FLASH_MS);
#endif
    app_led_off();
}

static void app_ir_carrier_on(void)
{
    (void)stc8h_pwm_set_duty(APP_IR_PWM_CHANNEL, APP_IR_PWM_DUTY);
    (void)stc8h_pwm_enable(APP_IR_PWM_CHANNEL);
}

static void app_ir_carrier_off(void)
{
    (void)stc8h_pwm_disable(APP_IR_PWM_CHANNEL);
    stc8h_gpio_write(BOARD_IR_PORT, BOARD_IR_PIN, 0u);
}

static void app_ir_send_tx(drv_ir_tx_nec_t *tx)
{
    stc8h_u16 duration_us;
    drv_ir_tx_level_t level;

    while (1) {
        level = drv_ir_tx_nec_next(tx, &duration_us);
        if (level == DRV_IR_TX_DONE) {
            app_ir_carrier_off();
            return;
        }

        if (level == DRV_IR_TX_MARK) {
            app_ir_carrier_on();
        } else {
            app_ir_carrier_off();
        }
        stc8h_delay_timer0_1t_us(duration_us);
    }
}

static void app_ir_send_command(stc8h_u8 command)
{
    drv_ir_tx_nec_t tx;

    drv_ir_tx_nec_begin(&tx, APP_IR_ADDRESS, command);
    app_ir_send_tx(&tx);
}

static void app_ir_send_repeat(void)
{
    drv_ir_tx_nec_t tx;

    drv_ir_tx_nec_repeat_begin(&tx);
    app_ir_send_tx(&tx);
}

static stc8h_u8 app_key_read_pin(stc8h_u8 key)
{
    switch (key) {
    case APP_KEY_POWER:
        return stc8h_gpio_read(BOARD_KEY_POWER_PORT, BOARD_KEY_POWER_PIN);
    case APP_KEY_UP:
        return stc8h_gpio_read(BOARD_KEY_UP_PORT, BOARD_KEY_UP_PIN);
    case APP_KEY_DOWN:
        return stc8h_gpio_read(BOARD_KEY_DOWN_PORT, BOARD_KEY_DOWN_PIN);
    case APP_KEY_FN1:
        return stc8h_gpio_read(BOARD_KEY_FN1_PORT, BOARD_KEY_FN1_PIN);
    case APP_KEY_FN2:
        return stc8h_gpio_read(BOARD_KEY_FN2_PORT, BOARD_KEY_FN2_PIN);
    default:
        return 1u;
    }
}

static stc8h_u8 app_key_is_pressed(stc8h_u8 key)
{
    return (app_key_read_pin(key) == APP_KEY_ACTIVE_LEVEL) ? 1u : 0u;
}

static stc8h_u8 app_key_command(stc8h_u8 key)
{
    switch (key) {
    case APP_KEY_POWER: return APP_CMD_POWER;
    case APP_KEY_UP: return APP_CMD_BRIGHTNESS_UP;
    case APP_KEY_DOWN: return APP_CMD_BRIGHTNESS_DOWN;
    case APP_KEY_FN1: return APP_CMD_FN1;
    case APP_KEY_FN2: return APP_CMD_FN2;
    default: return 0u;
    }
}

static stc8h_u8 app_key_can_repeat(stc8h_u8 key)
{
    return ((key == APP_KEY_UP) || (key == APP_KEY_DOWN)) ? 1u : 0u;
}

static stc8h_u8 app_scan_pressed_key(void)
{
    if (app_key_is_pressed(APP_KEY_POWER) != 0u) {
        return APP_KEY_POWER;
    }
    if (app_key_is_pressed(APP_KEY_UP) != 0u) {
        return APP_KEY_UP;
    }
    if (app_key_is_pressed(APP_KEY_DOWN) != 0u) {
        return APP_KEY_DOWN;
    }
    if (app_key_is_pressed(APP_KEY_FN1) != 0u) {
        return APP_KEY_FN1;
    }
    if (app_key_is_pressed(APP_KEY_FN2) != 0u) {
        return APP_KEY_FN2;
    }
    return APP_KEY_NONE;
}

static stc8h_u8 app_wait_key_pressed(stc8h_u8 key, stc8h_u16 wait_ms)
{
    stc8h_u16 remaining;
    stc8h_u16 step;

    remaining = wait_ms;
    while (remaining != 0u) {
        if (app_key_is_pressed(key) == 0u) {
            return 0u;
        }

        step = (remaining > APP_POLL_STEP_MS) ? APP_POLL_STEP_MS : remaining;
        app_delay_ms(step);
        remaining = (stc8h_u16)(remaining - step);
    }

    return app_key_is_pressed(key);
}

static void app_handle_repeat_key(stc8h_u8 key)
{
    if (app_wait_key_pressed(key, APP_REPEAT_FIRST_GAP_MS) == 0u) {
        return;
    }

    while (app_key_is_pressed(key) != 0u) {
        app_ir_send_repeat();
        (void)app_wait_key_pressed(key, APP_REPEAT_GAP_MS);
    }
}

static void app_wait_single_key_release(stc8h_u8 key)
{
    if (app_wait_key_pressed(key, APP_SINGLE_RELEASE_WINDOW_MS) == 0u) {
        app_sleep_while_key_pressed = APP_KEY_NONE;
    } else {
        app_sleep_while_key_pressed = key;
    }
}

static void app_handle_key(stc8h_u8 key)
{
    stc8h_u8 command;

    command = app_key_command(key);
    if (command == 0u) {
        return;
    }

    app_led_flash_once();
    app_ir_send_command(command);

    if (app_key_can_repeat(key) != 0u) {
        app_handle_repeat_key(key);
        app_sleep_while_key_pressed = APP_KEY_NONE;
    } else if (app_key_is_pressed(key) != 0u) {
        app_wait_single_key_release(key);
    }
}

static void app_clear_exti_flags(void)
{
    stc8h_exti_clear_flag(STC8H_EXTI_INT0);
    stc8h_exti_clear_flag(STC8H_EXTI_INT1);
    stc8h_exti_clear_flag(STC8H_EXTI_INT2);
    stc8h_exti_clear_flag(STC8H_EXTI_INT3);
    stc8h_exti_clear_flag(STC8H_EXTI_INT4);
}

static void app_configure_key_pin(stc8h_u8 port, stc8h_u8 pin)
{
    stc8h_gpio_write(port, pin, 1u);
    stc8h_gpio_set_mode(port, pin, STC8H_GPIO_MODE_QUASI);
}

static void app_io_init(void)
{
    stc8h_gpio_write(BOARD_IR_PORT, BOARD_IR_PIN, 0u);
    stc8h_gpio_set_mode(BOARD_IR_PORT, BOARD_IR_PIN, STC8H_GPIO_MODE_PUSH_PULL);

    app_led_off();
    stc8h_gpio_set_mode(BOARD_LED_PORT, BOARD_LED_PIN, STC8H_GPIO_MODE_PUSH_PULL);

    app_configure_key_pin(BOARD_KEY_POWER_PORT, BOARD_KEY_POWER_PIN);
    app_configure_key_pin(BOARD_KEY_UP_PORT, BOARD_KEY_UP_PIN);
    app_configure_key_pin(BOARD_KEY_DOWN_PORT, BOARD_KEY_DOWN_PIN);
    app_configure_key_pin(BOARD_KEY_FN1_PORT, BOARD_KEY_FN1_PIN);
    app_configure_key_pin(BOARD_KEY_FN2_PORT, BOARD_KEY_FN2_PIN);
}

static void app_ir_init(void)
{
    (void)stc8h_delay_timer0_1t_init();
    (void)stc8h_pwm_init(APP_IR_PWM_CHANNEL, APP_IR_PWM_PERIOD);
    (void)stc8h_pwm_set_duty(APP_IR_PWM_CHANNEL, 0u);
    app_ir_carrier_off();
}

static void app_exti_init(void)
{
    (void)stc8h_exti_configure(STC8H_EXTI_INT0, STC8H_EXTI_MODE_FALLING_EDGE);
    (void)stc8h_exti_configure(STC8H_EXTI_INT1, STC8H_EXTI_MODE_FALLING_EDGE);
    (void)stc8h_exti_configure(STC8H_EXTI_INT2, STC8H_EXTI_MODE_FALLING_EDGE);
    (void)stc8h_exti_configure(STC8H_EXTI_INT3, STC8H_EXTI_MODE_FALLING_EDGE);
    (void)stc8h_exti_configure(STC8H_EXTI_INT4, STC8H_EXTI_MODE_FALLING_EDGE);

    app_clear_exti_flags();

    stc8h_exti_enable(STC8H_EXTI_INT0);
    stc8h_exti_enable(STC8H_EXTI_INT1);
    stc8h_exti_enable(STC8H_EXTI_INT2);
    stc8h_exti_enable(STC8H_EXTI_INT3);
    stc8h_exti_enable(STC8H_EXTI_INT4);
}

static void app_board_init(void)
{
    app_woke = 0u;
    app_sleep_while_key_pressed = APP_KEY_NONE;
    app_io_init();
    app_ir_init();
    app_exti_init();
    stc8h_interrupt_enable_global();
}

static void app_enter_power_down(void)
{
    stc8h_u8 key;

    app_ir_carrier_off();
    app_led_off();

    stc8h_interrupt_disable_global();
    app_clear_exti_flags();
    app_woke = 0u;

    key = app_scan_pressed_key();
    if (key == APP_KEY_NONE) {
        app_sleep_while_key_pressed = APP_KEY_NONE;
    }

    if ((key == APP_KEY_NONE) || (key == app_sleep_while_key_pressed)) {
        stc8h_interrupt_enable_global();
        stc8h_power_down();
    } else {
        app_woke = 1u;
        stc8h_interrupt_enable_global();
    }
}

STC8H_INTERRUPT(app_int0_isr, STC8H_VECTOR_INT0)
{
    app_woke = 1u;
    stc8h_exti_clear_flag(STC8H_EXTI_INT0);
}

STC8H_INTERRUPT(app_int1_isr, STC8H_VECTOR_INT1)
{
    app_woke = 1u;
    stc8h_exti_clear_flag(STC8H_EXTI_INT1);
}

STC8H_INTERRUPT(app_int2_isr, STC8H_VECTOR_INT2)
{
    app_woke = 1u;
    stc8h_exti_clear_flag(STC8H_EXTI_INT2);
}

STC8H_INTERRUPT(app_int3_isr, STC8H_VECTOR_INT3)
{
    app_woke = 1u;
    stc8h_exti_clear_flag(STC8H_EXTI_INT3);
}

STC8H_INTERRUPT(app_int4_isr, STC8H_VECTOR_INT4)
{
    app_woke = 1u;
    stc8h_exti_clear_flag(STC8H_EXTI_INT4);
}

void main(void)
{
    stc8h_u8 key;

    app_board_init();

    while (1) {
        if (app_woke == 0u) {
            app_enter_power_down();
        }

        app_delay_ms(APP_DEBOUNCE_MS);
        if ((app_sleep_while_key_pressed != APP_KEY_NONE) &&
            (app_key_is_pressed(app_sleep_while_key_pressed) == 0u)) {
            app_sleep_while_key_pressed = APP_KEY_NONE;
        }

        key = app_scan_pressed_key();
        app_woke = 0u;

        if ((key != APP_KEY_NONE) && (key != app_sleep_while_key_pressed)) {
            app_handle_key(key);
        }
    }
}
