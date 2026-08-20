#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "app_config.h"

#define STC8H_GPIO_PORT_MASK ((1u << 1) | (1u << 3))

/* The current base library selects PWM resources by group and channel.
 * This remote has one fixed PWMA channel-1 output (P1.0) only. */
#define STC8H_PWM_GROUP_MASK             0x01u
#define STC8H_PWM_A_CHANNEL_MASK         0x01u
#define STC8H_PWM_B_CHANNEL_MASK         0x00u

/* The application sets a fixed, validated period and only writes bounded
 * duty values (0 or APP_IR_PWM_DUTY) before PWM output is enabled. */
#define STC8H_PWM_ENABLE_SET_DUTY_CHANNEL_CHECK 0
#define STC8H_PWM_ENABLE_SET_DUTY_CLAMP         0
#define STC8H_PWM_TRACK_PERIOD_PRESCALER        0

#endif
