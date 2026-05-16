BASE ?= ../../../Stc8hBase
BUILD := build
IR_UART_DEBUG ?= 1
IR_VERBOSE_DEBUG ?= 0
POWER_DOWN_ENABLE ?= 1
LOW_POWER_UNUSED_IO ?= 1
POWER_DOWN_GUARD_MS ?= 2000
GPIO_PORT_MASK ?= 0x0A
PWM_CHANNEL_MASK ?= 0x01

CC := sdcc
CFLAGS := -mmcs51 --std-sdcc11 \
	-Iboard \
	-Isrc \
	-I$(BASE)/core \
	-I$(BASE)/hal \
	-I$(BASE)/drivers \
	-DSTC8H_CONFIG_INCLUDE=\"board_config.h\" \
	-DSTC8H_PINS_INCLUDE=\"board_pins.h\" \
	-DAPP_IR_UART_DEBUG=$(IR_UART_DEBUG) \
	-DAPP_IR_VERBOSE_DEBUG=$(IR_VERBOSE_DEBUG) \
	-DAPP_POWER_DOWN_ENABLE=$(POWER_DOWN_ENABLE) \
	-DBOARD_LOW_POWER_UNUSED_IO=$(LOW_POWER_UNUSED_IO) \
	-DAPP_SLEEP_GUARD_MS=$(POWER_DOWN_GUARD_MS) \
	-DSTC8H_GPIO_PORT_MASK=$(GPIO_PORT_MASK) \
	-DSTC8H_PWM_CHANNEL_MASK=$(PWM_CHANNEL_MASK) \
	-DSTC8H_TIMER_ENABLE_1MS=0 \
	-DSTC8H_TIMER_ENABLE_TIMER0_FREE_RUN=1 \
	-DSTC8H_TIMER_ENABLE_TIMER0_RESET=1 \
	-DSTC8H_TIMER_ENABLE_RUN_CONTROL=1 \
	-DSTC8H_TIMER_ENABLE_INTERRUPT_CONTROL=0 \
	-DSTC8H_UART_ASSUME_UART1=1 \
	-DSTC8H_UART_ENABLE_WRITE_RAM=0 \
	-DSTC8H_UART_ENABLE_RX=0 \
	-DDRV_IR_RX_ENABLE_PULSE=1 \
	-DDRV_IR_RX_ENABLE_FALLING=0

SRCS := \
	src/main.c \
	src/app_light.c \
	src/app_ir.c \
	board/board_init.c \
	$(BASE)/core/stc8h_interrupt.c \
	$(BASE)/hal/stc8h_gpio.c \
	$(BASE)/hal/stc8h_timer.c \
	$(BASE)/hal/stc8h_pwm.c \
	$(BASE)/hal/stc8h_exti.c \
	$(BASE)/drivers/drv_ir_rx.c

ifeq ($(IR_UART_DEBUG),1)
SRCS += $(BASE)/hal/stc8h_uart.c
endif

TARGET := $(BUILD)/ir_lamp.ihx
DEBUG_IR_TARGET := $(BUILD)/debug_ir.ihx
OBJS := $(patsubst %.c,$(BUILD)/%.rel,$(notdir $(SRCS)))
DEBUG_IR_SRCS := \
	src/debug_ir_main.c \
	board/board_init.c \
	$(BASE)/core/stc8h_interrupt.c \
	$(BASE)/hal/stc8h_gpio.c \
	$(BASE)/hal/stc8h_timer.c \
	$(BASE)/hal/stc8h_exti.c \
	$(BASE)/hal/stc8h_uart.c \
	$(BASE)/drivers/drv_ir_rx.c
DEBUG_IR_OBJS := $(patsubst %.c,$(BUILD)/debug_%.rel,$(notdir $(DEBUG_IR_SRCS)))
VPATH := src board $(BASE)/core $(BASE)/hal $(BASE)/drivers

.PHONY: all debug-ir clean

all: $(TARGET)

debug-ir: $(DEBUG_IR_TARGET)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.rel: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/debug_%.rel: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS) | $(BUILD)
	$(CC) $(CFLAGS) --out-fmt-ihx -o $(TARGET) $(OBJS)

$(DEBUG_IR_TARGET): $(DEBUG_IR_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) --out-fmt-ihx -o $(DEBUG_IR_TARGET) $(DEBUG_IR_OBJS)

clean:
	rm -rf $(BUILD)
