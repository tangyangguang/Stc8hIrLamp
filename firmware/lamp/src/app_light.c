#include "app_light.h"
#include "board_pins.h"
#include "stc8h_gpio.h"
#include "stc8h_pwm.h"

#define APP_LIGHT_PWM_CHANNEL STC8H_PWM_CHANNEL_1
#define APP_LIGHT_PWM_FREQ_HZ 1000UL
#define APP_LIGHT_PWM_PERIOD ((stc8h_u16)((STC8H_SYSCLK_HZ / APP_LIGHT_PWM_FREQ_HZ) - 1UL))
#define APP_LIGHT_DUTY(percent) \
    ((stc8h_u16)((((STC8H_SYSCLK_HZ / APP_LIGHT_PWM_FREQ_HZ) - 1UL) * (percent)) / 100UL))
#define APP_LIGHT_DEFAULT_LEVEL_INDEX 8u
#define APP_LIGHT_REPEAT_INTERVAL_MS 100u
#define APP_LIGHT_LED_FLASH_MS 50u
#define APP_LIGHT_TIMER_FEEDBACK_MS 100u
#define APP_LIGHT_TIMER_15_MIN_SEC 900u
#define APP_LIGHT_TIMER_60_MIN_SEC 3600u

static const STC8H_CODE stc8h_u16 brightness_duty[] = {
    APP_LIGHT_DUTY(1UL),
    APP_LIGHT_DUTY(2UL),
    APP_LIGHT_DUTY(3UL),
    APP_LIGHT_DUTY(5UL),
    APP_LIGHT_DUTY(9UL),
    APP_LIGHT_DUTY(15UL),
    APP_LIGHT_DUTY(20UL),
    APP_LIGHT_DUTY(25UL),
    APP_LIGHT_DUTY(30UL),
    APP_LIGHT_DUTY(40UL),
    APP_LIGHT_DUTY(50UL),
    APP_LIGHT_DUTY(60UL),
    APP_LIGHT_DUTY(70UL),
    APP_LIGHT_DUTY(80UL),
    APP_LIGHT_DUTY(90UL)
};

static stc8h_u8 light_on;
static stc8h_u8 level_index;
static stc8h_u16 countdown_sec;
static stc8h_u16 second_ms;
static stc8h_u16 repeat_ms;
static stc8h_u8 led_flash_ms;
static stc8h_u8 timer_feedback_ms;

static void app_light_led_set(stc8h_u8 on)
{
    stc8h_gpio_write(BOARD_STATUS_LED_PORT, BOARD_STATUS_LED_PIN, on);
}

static void app_light_apply_pwm(void)
{
    (void)stc8h_pwm_set_duty(APP_LIGHT_PWM_CHANNEL, brightness_duty[level_index]);
}

static void app_light_output_on(void)
{
    app_light_apply_pwm();
    (void)stc8h_pwm_enable(APP_LIGHT_PWM_CHANNEL);
}

static void app_light_output_off(void)
{
    (void)stc8h_pwm_disable(APP_LIGHT_PWM_CHANNEL);
    stc8h_gpio_write(BOARD_LAMP_PWM_PORT, BOARD_LAMP_PWM_PIN, 0u);
}

static void app_light_turn_on(void)
{
    light_on = 1u;
    countdown_sec = 0u;
    second_ms = 0u;
    timer_feedback_ms = 0u;
    app_light_output_on();
}

static void app_light_turn_off(void)
{
    light_on = 0u;
    countdown_sec = 0u;
    second_ms = 0u;
    timer_feedback_ms = 0u;
    app_light_output_off();
}

static void app_light_level_up(void)
{
    if (light_on == 0u) {
        return;
    }
    if (level_index < ((stc8h_u8)(sizeof(brightness_duty) / sizeof(brightness_duty[0])) - 1u)) {
        ++level_index;
    }
    app_light_apply_pwm();
}

static void app_light_level_down(void)
{
    if (light_on == 0u) {
        return;
    }
    if (level_index > 0u) {
        --level_index;
    }
    app_light_apply_pwm();
}

static void app_light_set_timer(stc8h_u16 seconds)
{
    if (light_on == 0u) {
        return;
    }
    countdown_sec = seconds;
    second_ms = 0u;
    timer_feedback_ms = APP_LIGHT_TIMER_FEEDBACK_MS;
    app_light_output_off();
}

static void app_light_start_led_flash(void)
{
    led_flash_ms = APP_LIGHT_LED_FLASH_MS;
    app_light_led_set(1u);
}

void app_light_init(void)
{
    light_on = 0u;
    level_index = APP_LIGHT_DEFAULT_LEVEL_INDEX;
    countdown_sec = 0u;
    second_ms = 0u;
    repeat_ms = APP_LIGHT_REPEAT_INTERVAL_MS;
    led_flash_ms = 0u;
    timer_feedback_ms = 0u;

    (void)stc8h_pwm_init(APP_LIGHT_PWM_CHANNEL, APP_LIGHT_PWM_PERIOD);
    app_light_turn_off();
    app_light_led_set(0u);
}

void app_light_tick_ms(void)
{
    if (repeat_ms < APP_LIGHT_REPEAT_INTERVAL_MS) {
        ++repeat_ms;
    }

    if (led_flash_ms != 0u) {
        --led_flash_ms;
        if (led_flash_ms == 0u) {
            app_light_led_set(0u);
        }
    }

    if (timer_feedback_ms != 0u) {
        --timer_feedback_ms;
        if ((timer_feedback_ms == 0u) && (light_on != 0u)) {
            app_light_output_on();
        }
    }

    if ((light_on != 0u) && (countdown_sec != 0u)) {
        ++second_ms;
        if (second_ms >= 1000u) {
            second_ms = 0u;
            --countdown_sec;
            if (countdown_sec == 0u) {
                app_light_turn_off();
            }
        }
    }
}

void app_light_handle_ir_event(const app_ir_event_t *event)
{
    if ((event == 0) || (event->command == APP_IR_COMMAND_NONE)) {
        return;
    }

    if (event->repeat != 0u) {
        if (repeat_ms < APP_LIGHT_REPEAT_INTERVAL_MS) {
            return;
        }
        repeat_ms = 0u;
    } else {
        repeat_ms = 0u;
        app_light_start_led_flash();
    }

    if (event->command == APP_IR_COMMAND_POWER) {
        if (event->repeat == 0u) {
            if (light_on == 0u) {
                app_light_turn_on();
            } else {
                app_light_turn_off();
            }
        }
    } else if (event->command == APP_IR_COMMAND_BRIGHTER) {
        app_light_level_up();
    } else if (event->command == APP_IR_COMMAND_DIMMER) {
        app_light_level_down();
    } else if ((event->command == APP_IR_COMMAND_TIMER_15_MIN) && (event->repeat == 0u)) {
        app_light_set_timer(APP_LIGHT_TIMER_15_MIN_SEC);
    } else if ((event->command == APP_IR_COMMAND_TIMER_60_MIN) && (event->repeat == 0u)) {
        app_light_set_timer(APP_LIGHT_TIMER_60_MIN_SEC);
    }
}

stc8h_u8 app_light_is_on(void)
{
    return light_on;
}
