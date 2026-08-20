#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define STC8H_SYSCLK_HZ 6000000UL
#define STC8H_CHIP_STC8H1K08 1

#define APP_IR_ADDRESS 0x01u

#define APP_CMD_POWER           0x11u
#define APP_CMD_BRIGHTNESS_UP   0x22u
#define APP_CMD_BRIGHTNESS_DOWN 0x33u
#define APP_CMD_FN1             0x51u
#define APP_CMD_FN2             0x52u

#define APP_KEY_ACTIVE_LEVEL 0u

#define APP_DEBOUNCE_MS             10u
#ifndef APP_LED_FEEDBACK_ENABLE
#define APP_LED_FEEDBACK_ENABLE      1u
#endif
#define APP_LED_FLASH_MS             5u
#define APP_SINGLE_RELEASE_WINDOW_MS 80u
#define APP_REPEAT_FIRST_GAP_MS     40u
#define APP_REPEAT_GAP_MS           96u

#define APP_IR_PWM_GROUP   STC8H_PWM_GROUP_A
#define APP_IR_PWM_CHANNEL STC8H_PWM_CHANNEL_1
#define APP_IR_PWM_PIN     STC8H_PWM_PIN_PWM1_P10
#define APP_IR_PWM_PRESCALER 0u
#define APP_IR_PWM_PERIOD  158u
#ifndef APP_IR_PWM_DUTY
#define APP_IR_PWM_DUTY    105u
#endif

#if APP_IR_PWM_DUTY > APP_IR_PWM_PERIOD
#error "APP_IR_PWM_DUTY must not exceed APP_IR_PWM_PERIOD"
#endif

#ifndef APP_IR_ACTIVE_LEVEL
#define APP_IR_ACTIVE_LEVEL 1u
#endif

#if APP_IR_ACTIVE_LEVEL
#define APP_IR_IDLE_LEVEL 0u
#else
#define APP_IR_IDLE_LEVEL 1u
#endif

#endif
