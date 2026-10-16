PROJECT_ROOT := ../../..
FIRMWARE_DIR := $(PROJECT_ROOT)/firmware
SDK_DIR := $(FIRMWARE_DIR)/vendor/py32f002b_firmware

TARGET := sp01_py32f002b

ifeq ($(TOUCH_DIAG_TEST),1)
BUILD_DIR ?= build-touch-diag-test001
else ifeq ($(RGB_WHITE_BALANCE_TEST),1)
BUILD_DIR ?= build-rgb-white-balance-test001
else ifeq ($(RGB_TUNER_TEST),1)
BUILD_DIR ?= build-rgb-tuner-test001
else ifeq ($(RGB_TRISTATE),1)
BUILD_DIR ?= build-rgb-tristate-scan-test001
else ifeq ($(RGB_COMBO_SCAN_TEST),1)
BUILD_DIR ?= build-rgb-combo-scan-test001
else ifeq ($(RGB_ALL_ON_TEST),1)
BUILD_DIR ?= build-rgb-all-on-test001
else ifeq ($(RGB_COMBO_PA5_PA1_TEST),1)
BUILD_DIR ?= build-rgb-combo-pa5-pa1-test001
else ifeq ($(RGB_ISOLATED_PB3_TEST),1)
BUILD_DIR ?= build-rgb-isolated-pb3-test001
else ifeq ($(RGB_ISOLATED_PA5_TEST),1)
BUILD_DIR ?= build-rgb-isolated-pa5-test001
else ifeq ($(RGB_SINGLE_TEST),1)
BUILD_DIR ?= build-rgb-single-test001
else ifeq ($(RGB_TEST),1)
BUILD_DIR ?= build-rgb-test001
else ifeq ($(LOW_WATER_TEST),1)
BUILD_DIR ?= build-low-water-test001
else ifeq ($(PUMP_ADC_HOLD_TEST),1)
BUILD_DIR ?= build-pump-adc-hold-test002
else ifeq ($(PUMP_ADC_TEST),1)
BUILD_DIR ?= build-pump-adc-test001
else ifeq ($(FULL_TEST),1)
BUILD_DIR ?= build-full-test001
else ifeq ($(SPEC_TEST003),1)
BUILD_DIR ?= build-spec-test003
else ifeq ($(SPEC_TEST006),1)
BUILD_DIR ?= build-spec-test006
else ifeq ($(SPEC_TEST007),1)
BUILD_DIR ?= build-spec-test007
else ifeq ($(SPEC_TEST005),1)
BUILD_DIR ?= build-spec-test005
else ifeq ($(SPEC_TEST004),1)
BUILD_DIR ?= build-spec-test004
else ifeq ($(SPEC_TEST002),1)
BUILD_DIR ?= build-spec-test002
else ifeq ($(SPEC_TEST),1)
BUILD_DIR ?= build-spec-test001
else ifeq ($(BATTERY_PUMP_TEST),1)
BUILD_DIR ?= build-battery-pump-test
else ifeq ($(BATTERY_TEST),1)
BUILD_DIR ?= build-battery-test
else ifeq ($(RADAR_TEST),1)
BUILD_DIR ?= build-radar-test
else ifeq ($(PUMP_TEST),1)
BUILD_DIR ?= build-pump-test
else ifeq ($(BENCH_NO_PUMP),1)
BUILD_DIR ?= build-bench-no-pump
else
BUILD_DIR ?= build
endif

ifneq ($(filter 1,$(RGB_WHITE_BALANCE_TEST)),)
ifneq ($(filter 1,$(RELEASE_BUILD) $(BENCH_NO_PUMP) $(FULL_TEST) $(LOW_WATER_TEST) $(RADAR_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(PUMP_ADC_TEST) $(PUMP_ADC_HOLD_TEST) $(RGB_TEST) $(RGB_SINGLE_TEST) $(RGB_ISOLATED_PA5_TEST) $(RGB_ISOLATED_PB3_TEST) $(RGB_COMBO_PA5_PA1_TEST) $(RGB_COMBO_SCAN_TEST) $(RGB_TRISTATE) $(RGB_ALL_ON_TEST) $(SPEC_TEST)),)
$(error RGB_WHITE_BALANCE_TEST cannot be combined with other build profiles)
endif
endif

ifneq ($(filter 1,$(RGB_TUNER_TEST)),)
ifneq ($(filter 1,$(RELEASE_BUILD) $(BENCH_NO_PUMP) $(FULL_TEST) $(LOW_WATER_TEST) $(RADAR_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(PUMP_ADC_TEST) $(PUMP_ADC_HOLD_TEST) $(RGB_TEST) $(RGB_SINGLE_TEST) $(RGB_ISOLATED_PA5_TEST) $(RGB_ISOLATED_PB3_TEST) $(RGB_COMBO_PA5_PA1_TEST) $(RGB_COMBO_SCAN_TEST) $(RGB_TRISTATE) $(RGB_ALL_ON_TEST) $(RGB_WHITE_BALANCE_TEST) $(SPEC_TEST) $(SPEC_TEST002) $(SPEC_TEST003) $(SPEC_TEST004) $(SPEC_TEST005) $(SPEC_TEST006) $(SPEC_TEST007)),)
$(error RGB_TUNER_TEST cannot be combined with other build profiles)
endif
endif

ifeq ($(RGB_TRISTATE),1)
ifneq ($(filter 1,$(RGB_COMBO_SCAN_TEST)),1)
$(error RGB_TRISTATE requires RGB_COMBO_SCAN_TEST=1)
endif
endif

ifneq ($(filter 1,$(RGB_ALL_ON_TEST)),)
ifneq ($(filter 1,$(RELEASE_BUILD) $(FULL_TEST) $(LOW_WATER_TEST) $(PUMP_ADC_TEST) $(PUMP_ADC_HOLD_TEST) $(BENCH_NO_PUMP) $(RADAR_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(RGB_TEST) $(RGB_SINGLE_TEST) $(RGB_ISOLATED_PA5_TEST) $(RGB_ISOLATED_PB3_TEST) $(RGB_COMBO_PA5_PA1_TEST) $(RGB_COMBO_SCAN_TEST) $(RGB_TRISTATE)),)
$(error RGB_ALL_ON_TEST cannot be combined with other build profiles)
endif
endif

ifneq ($(filter 1,$(RELEASE_BUILD)),)
ifneq ($(filter 1,$(RGB_SINGLE_TEST) $(RGB_TEST) $(RGB_ISOLATED_PA5_TEST) $(RGB_ISOLATED_PB3_TEST) $(RGB_COMBO_PA5_PA1_TEST) $(RGB_COMBO_SCAN_TEST) $(LOW_WATER_TEST) $(FULL_TEST) $(BENCH_NO_PUMP) $(RADAR_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(PUMP_ADC_TEST) $(PUMP_ADC_HOLD_TEST)),)
$(error Bench test profiles cannot be combined with RELEASE_BUILD)
endif
endif

ifneq ($(filter 1,$(RGB_SINGLE_TEST)),)
ifneq ($(filter 1,$(RGB_TEST) $(RGB_ISOLATED_PA5_TEST) $(RGB_ISOLATED_PB3_TEST) $(RGB_COMBO_PA5_PA1_TEST) $(RGB_COMBO_SCAN_TEST) $(FULL_TEST) $(BENCH_NO_PUMP) $(RADAR_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(RELEASE_BUILD)),)
$(error RGB_SINGLE_TEST cannot be combined with other build profiles)
endif
ifeq ($(filter PB3 PA5 PA1,$(RGB_CHANNEL)),)
$(error RGB_CHANNEL must be PB3, PA5, or PA1 when RGB_SINGLE_TEST=1)
endif
endif

ifneq ($(filter 1,$(RGB_ISOLATED_PA5_TEST)),)
ifneq ($(filter 1,$(RGB_TEST) $(RGB_SINGLE_TEST) $(RGB_ISOLATED_PB3_TEST) $(RGB_COMBO_PA5_PA1_TEST) $(RGB_COMBO_SCAN_TEST) $(FULL_TEST) $(BENCH_NO_PUMP) $(RADAR_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(RELEASE_BUILD)),)
$(error RGB_ISOLATED_PA5_TEST cannot be combined with other build profiles)
endif
endif

ifneq ($(filter 1,$(RGB_ISOLATED_PB3_TEST)),)
ifneq ($(filter 1,$(RGB_TEST) $(RGB_SINGLE_TEST) $(RGB_ISOLATED_PA5_TEST) $(RGB_COMBO_PA5_PA1_TEST) $(RGB_COMBO_SCAN_TEST) $(FULL_TEST) $(BENCH_NO_PUMP) $(RADAR_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(RELEASE_BUILD)),)
$(error RGB_ISOLATED_PB3_TEST cannot be combined with other build profiles)
endif
endif

ifneq ($(filter 1,$(RGB_TEST)),)
ifneq ($(filter 1,$(RGB_SINGLE_TEST) $(RGB_ISOLATED_PA5_TEST) $(RGB_ISOLATED_PB3_TEST) $(FULL_TEST) $(BENCH_NO_PUMP) $(RADAR_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(RELEASE_BUILD)),)
$(error RGB_TEST cannot be combined with other build profiles)
endif
endif

ifneq ($(filter 1,$(FULL_TEST)),)
ifneq ($(filter 1,$(BENCH_NO_PUMP) $(RADAR_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(RELEASE_BUILD)),)
$(error FULL_TEST cannot be combined with other build profiles)
endif

endif

ifneq ($(filter 1,$(SPEC_TEST)),)
ifneq ($(filter 1,$(SPEC_TEST002) $(SPEC_TEST003) $(SPEC_TEST004) $(SPEC_TEST005) $(SPEC_TEST006) $(SPEC_TEST007) $(RELEASE_BUILD) $(FULL_TEST) $(LOW_WATER_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(RGB_TEST) $(RGB_SINGLE_TEST) $(RGB_ISOLATED_PA5_TEST) $(RGB_ISOLATED_PB3_TEST) $(RGB_COMBO_PA5_PA1_TEST) $(RGB_COMBO_SCAN_TEST) $(RGB_ALL_ON_TEST)),)
$(error SPEC_TEST cannot be combined with other build profiles)
endif
endif

ifneq ($(filter 1,$(SPEC_TEST002)),)
ifneq ($(filter 1,$(SPEC_TEST) $(SPEC_TEST003) $(SPEC_TEST004) $(SPEC_TEST005) $(SPEC_TEST006) $(SPEC_TEST007) $(RELEASE_BUILD) $(FULL_TEST) $(LOW_WATER_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(RGB_TEST) $(RGB_SINGLE_TEST) $(RGB_ISOLATED_PA5_TEST) $(RGB_ISOLATED_PB3_TEST) $(RGB_COMBO_PA5_PA1_TEST) $(RGB_COMBO_SCAN_TEST) $(RGB_ALL_ON_TEST)),)
$(error SPEC_TEST002 cannot be combined with other build profiles)
endif
endif

ifneq ($(filter 1,$(SPEC_TEST003)),)
ifneq ($(filter 1,$(SPEC_TEST) $(SPEC_TEST002) $(SPEC_TEST004) $(SPEC_TEST005) $(SPEC_TEST006) $(SPEC_TEST007) $(RELEASE_BUILD) $(FULL_TEST) $(LOW_WATER_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(RGB_TEST) $(RGB_SINGLE_TEST) $(RGB_ISOLATED_PA5_TEST) $(RGB_ISOLATED_PB3_TEST) $(RGB_COMBO_PA5_PA1_TEST) $(RGB_COMBO_SCAN_TEST) $(RGB_ALL_ON_TEST)),)
$(error SPEC_TEST003 cannot be combined with other build profiles)
endif
endif

ifneq ($(filter 1,$(SPEC_TEST004)),)
ifneq ($(filter 1,$(SPEC_TEST) $(SPEC_TEST002) $(SPEC_TEST003) $(SPEC_TEST005) $(SPEC_TEST006) $(SPEC_TEST007) $(RELEASE_BUILD) $(FULL_TEST) $(LOW_WATER_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(RGB_TEST) $(RGB_SINGLE_TEST) $(RGB_ISOLATED_PA5_TEST) $(RGB_ISOLATED_PB3_TEST) $(RGB_COMBO_PA5_PA1_TEST) $(RGB_COMBO_SCAN_TEST) $(RGB_ALL_ON_TEST)),)
$(error SPEC_TEST004 cannot be combined with other build profiles)
endif
endif

ifneq ($(filter 1,$(SPEC_TEST005)),)
ifneq ($(filter 1,$(SPEC_TEST) $(SPEC_TEST002) $(SPEC_TEST003) $(SPEC_TEST004) $(SPEC_TEST006) $(SPEC_TEST007) $(RELEASE_BUILD) $(FULL_TEST) $(LOW_WATER_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(RGB_TEST) $(RGB_SINGLE_TEST) $(RGB_ISOLATED_PA5_TEST) $(RGB_ISOLATED_PB3_TEST) $(RGB_COMBO_PA5_PA1_TEST) $(RGB_COMBO_SCAN_TEST) $(RGB_ALL_ON_TEST)),)
$(error SPEC_TEST005 cannot be combined with other build profiles)
endif
endif

ifneq ($(filter 1,$(SPEC_TEST007)),)
ifneq ($(filter 1,$(SPEC_TEST) $(SPEC_TEST002) $(SPEC_TEST003) $(SPEC_TEST004) $(SPEC_TEST005) $(SPEC_TEST006) $(RELEASE_BUILD) $(FULL_TEST) $(LOW_WATER_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(RGB_TEST) $(RGB_SINGLE_TEST) $(RGB_ISOLATED_PA5_TEST) $(RGB_ISOLATED_PB3_TEST) $(RGB_COMBO_PA5_PA1_TEST) $(RGB_COMBO_SCAN_TEST) $(RGB_ALL_ON_TEST)),)
$(error SPEC_TEST007 cannot be combined with other build profiles)
endif
endif

ifneq ($(filter 1,$(SPEC_TEST006)),)
ifneq ($(filter 1,$(SPEC_TEST) $(SPEC_TEST002) $(SPEC_TEST003) $(SPEC_TEST004) $(SPEC_TEST005) $(SPEC_TEST007) $(RELEASE_BUILD) $(FULL_TEST) $(LOW_WATER_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(RGB_TEST) $(RGB_SINGLE_TEST) $(RGB_ISOLATED_PA5_TEST) $(RGB_ISOLATED_PB3_TEST) $(RGB_COMBO_PA5_PA1_TEST) $(RGB_COMBO_SCAN_TEST) $(RGB_ALL_ON_TEST)),)
$(error SPEC_TEST006 cannot be combined with other build profiles)
endif
endif

ifneq ($(filter 1,$(LOW_WATER_TEST)),)
ifneq ($(filter 1,$(RGB_SINGLE_TEST) $(RGB_TEST) $(RGB_ISOLATED_PA5_TEST) $(RGB_ISOLATED_PB3_TEST) $(RGB_COMBO_PA5_PA1_TEST) $(RGB_COMBO_SCAN_TEST) $(FULL_TEST) $(BENCH_NO_PUMP) $(RADAR_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(RELEASE_BUILD)),)
$(error LOW_WATER_TEST cannot be combined with other build profiles)
endif
endif

ifneq ($(filter 1,$(RADAR_TEST)),)
ifneq ($(filter 1,$(BENCH_NO_PUMP)),)
$(error RADAR_TEST selects its own no-pump profile)
endif
endif

ifneq ($(filter 1,$(PUMP_TEST)),)
ifneq ($(filter 1,$(BENCH_NO_PUMP) $(RADAR_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST)),)
$(error PUMP_TEST cannot be combined with other test profiles)
endif
endif

ifneq ($(filter 1,$(BATTERY_TEST)),)
ifneq ($(filter 1,$(BENCH_NO_PUMP) $(RADAR_TEST) $(PUMP_TEST) $(BATTERY_PUMP_TEST)),)
$(error BATTERY_TEST cannot be combined with other test profiles)
endif
endif

ifneq ($(filter 1,$(BATTERY_PUMP_TEST)),)
ifneq ($(filter 1,$(BENCH_NO_PUMP) $(RADAR_TEST) $(PUMP_TEST) $(BATTERY_TEST)),)
$(error BATTERY_PUMP_TEST cannot be combined with other test profiles)
endif
endif

ifdef GCC_PATH
CC := $(GCC_PATH)/arm-none-eabi-gcc
AS := $(GCC_PATH)/arm-none-eabi-gcc -x assembler-with-cpp
OBJCOPY := $(GCC_PATH)/arm-none-eabi-objcopy
SIZE := $(GCC_PATH)/arm-none-eabi-size
else
CC := arm-none-eabi-gcc
AS := arm-none-eabi-gcc -x assembler-with-cpp
OBJCOPY := arm-none-eabi-objcopy
SIZE := arm-none-eabi-size
endif

CPU := -mcpu=cortex-m0plus -mthumb
COMMON_FLAGS := $(CPU) -ffunction-sections -fdata-sections -Wall -Wextra
CFLAGS := $(COMMON_FLAGS) -O0 -g3 -MMD -MP
ASFLAGS := $(COMMON_FLAGS)

C_DEFS := \
    -DUSE_HAL_DRIVER \
    -DPY32F002Bx5 \
    -DSP01_PY32_VENDOR_PORT

ifdef RELEASE_BUILD
C_DEFS += -DSP01_RELEASE_BUILD
endif

ifeq ($(BENCH_NO_PUMP),1)
C_DEFS += -DSP01_BENCH_NO_PUMP
endif

ifeq ($(RADAR_TEST),1)
C_DEFS += -DSP01_RADAR_TEST -DSP01_BENCH_NO_PUMP
endif

ifeq ($(PUMP_TEST),1)
C_DEFS += -DSP01_PUMP_TEST
endif

ifeq ($(BATTERY_TEST),1)
C_DEFS += -DSP01_BATTERY_TEST
endif

ifeq ($(BATTERY_PUMP_TEST),1)
C_DEFS += -DSP01_BATTERY_PUMP_TEST
endif

ifeq ($(TOUCH_DIAG_TEST),1)
ifneq ($(filter 1,$(RELEASE_BUILD) $(BENCH_NO_PUMP) $(FULL_TEST) $(LOW_WATER_TEST) $(SPEC_TEST) $(SPEC_TEST002) $(SPEC_TEST003) $(SPEC_TEST004) $(RADAR_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(PUMP_ADC_TEST) $(PUMP_ADC_HOLD_TEST) $(RGB_TEST) $(RGB_SINGLE_TEST) $(RGB_ISOLATED_PA5_TEST) $(RGB_ISOLATED_PB3_TEST) $(RGB_COMBO_PA5_PA1_TEST) $(RGB_COMBO_SCAN_TEST) $(RGB_TRISTATE) $(RGB_ALL_ON_TEST)),)
$(error TOUCH_DIAG_TEST cannot be combined with other build profiles)
endif
C_DEFS += -DSP01_TOUCH_DIAG_TEST
endif

ifeq ($(FULL_TEST),1)
C_DEFS += -DSP01_FULL_TEST
endif

ifeq ($(SPEC_TEST),1)
C_DEFS += -DSP01_SPEC_TEST
endif

ifeq ($(SPEC_TEST002),1)
C_DEFS += -DSP01_SPEC_TEST -DSP01_SPEC_TEST002
endif

ifeq ($(SPEC_TEST003),1)
C_DEFS += -DSP01_SPEC_TEST -DSP01_SPEC_TEST003
endif

ifeq ($(SPEC_TEST004),1)
C_DEFS += -DSP01_SPEC_TEST -DSP01_SPEC_TEST004
endif

ifeq ($(SPEC_TEST005),1)
C_DEFS += -DSP01_SPEC_TEST -DSP01_SPEC_TEST005
endif

ifeq ($(SPEC_TEST006),1)
C_DEFS += -DSP01_SPEC_TEST -DSP01_SPEC_TEST006
endif

ifeq ($(SPEC_TEST007),1)
C_DEFS += -DSP01_SPEC_TEST -DSP01_SPEC_TEST007
endif

ifeq ($(LOW_WATER_TEST),1)
C_DEFS += -DSP01_LOW_WATER_TEST -DSP01_FULL_TEST -DSP01_PUMP_DIAGNOSTICS_ENABLED=1
endif

ifeq ($(PUMP_ADC_TEST),1)
C_DEFS += -DSP01_PUMP_ADC_TEST -DSP01_PUMP_DIAGNOSTICS_ENABLED=1
ifdef PUMP_ADC_CHANNEL
C_DEFS += -DSP01_PUMP_ADC_CHANNEL=$(PUMP_ADC_CHANNEL)
endif
endif

ifeq ($(PUMP_ADC_HOLD_TEST),1)
ifneq ($(filter 1,$(PUMP_ADC_TEST) $(RELEASE_BUILD) $(FULL_TEST) $(LOW_WATER_TEST) $(BENCH_NO_PUMP) $(RADAR_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(RGB_TEST) $(RGB_SINGLE_TEST) $(RGB_ISOLATED_PA5_TEST) $(RGB_ISOLATED_PB3_TEST) $(RGB_COMBO_PA5_PA1_TEST) $(RGB_COMBO_SCAN_TEST) $(RGB_TRISTATE)),)
$(error PUMP_ADC_HOLD_TEST cannot be combined with other build profiles)
endif
C_DEFS += -DSP01_PUMP_ADC_HOLD_TEST
ifdef PUMP_ADC_CHANNEL
C_DEFS += -DSP01_PUMP_ADC_CHANNEL=$(PUMP_ADC_CHANNEL)
endif
endif

ifeq ($(RGB_TEST),1)
C_DEFS += -DSP01_RGB_TEST
endif

ifeq ($(RGB_SINGLE_TEST),1)
C_DEFS += -DSP01_RGB_SINGLE_TEST -DSP01_RGB_CHANNEL_$(RGB_CHANNEL)
ifeq ($(RGB_SINGLE_ACTIVE_HIGH),1)
C_DEFS += -DSP01_RGB_SINGLE_ACTIVE_HIGH
endif
endif

ifeq ($(RGB_ISOLATED_PA5_TEST),1)
C_DEFS += -DSP01_RGB_ISOLATED_PA5_TEST
endif

ifeq ($(RGB_ISOLATED_PB3_TEST),1)
C_DEFS += -DSP01_RGB_ISOLATED_PB3_TEST
endif

ifeq ($(RGB_COMBO_PA5_PA1_TEST),1)
C_DEFS += -DSP01_RGB_COMBO_PA5_PA1_TEST
endif

ifeq ($(RGB_COMBO_SCAN_TEST),1)
C_DEFS += -DSP01_RGB_COMBO_SCAN_TEST
endif

ifeq ($(RGB_ALL_ON_TEST),1)
C_DEFS += -DSP01_RGB_ALL_ON_TEST
endif

ifeq ($(RGB_WHITE_BALANCE_TEST),1)
C_DEFS += -DSP01_RGB_WHITE_BALANCE_TEST
endif

ifeq ($(RGB_TUNER_TEST),1)
C_DEFS += -DSP01_RGB_TUNER_TEST
endif

ifeq ($(RGB_TRISTATE),1)
C_DEFS += -DSP01_RGB_TRISTATE
endif

C_INCLUDES := \
    -I$(FIRMWARE_DIR) \
    -I$(FIRMWARE_DIR)/app \
    -I$(FIRMWARE_DIR)/platform/py32f002b \
    -I$(SDK_DIR)/Drivers/CMSIS/Include \
    -I$(SDK_DIR)/Drivers/CMSIS/Device/PY32F0xx/Include \
    -I$(SDK_DIR)/Drivers/PY32F002B_HAL_Driver/Inc

C_SOURCES := \
    $(FIRMWARE_DIR)/main.c \
    $(FIRMWARE_DIR)/app/sp01_app.c \
    $(FIRMWARE_DIR)/app/sp01_buzzer_pattern.c \
    $(FIRMWARE_DIR)/app/sp01_power_monitor.c \
    $(FIRMWARE_DIR)/app/sp01_signal_filter.c \
    $(FIRMWARE_DIR)/app/sp01_indicator.c \
    $(FIRMWARE_DIR)/app/sp01_touch_button.c \
    $(FIRMWARE_DIR)/app/sp01_radar_input.c \
    $(FIRMWARE_DIR)/app/sp01_charge_monitor.c \
    $(FIRMWARE_DIR)/app/sp01_pump_monitor.c \
    $(FIRMWARE_DIR)/app/sp01_pump_test.c \
    $(FIRMWARE_DIR)/app/sp01_pump_adc_test.c \
    $(FIRMWARE_DIR)/app/sp01_pump_adc_hold_test.c \
    $(FIRMWARE_DIR)/app/sp01_battery_test.c \
    $(FIRMWARE_DIR)/app/sp01_battery_pump_test.c \
    $(FIRMWARE_DIR)/app/sp01_full_test.c \
    $(FIRMWARE_DIR)/app/sp01_spec_product.c \
    $(FIRMWARE_DIR)/app/sp01_rgb_test.c \
    $(FIRMWARE_DIR)/app/sp01_touch_diag_test.c \
    $(FIRMWARE_DIR)/platform/py32f002b/board_port.c \
    $(FIRMWARE_DIR)/platform/py32f002b/vendor_port.c \
    $(FIRMWARE_DIR)/platform/py32f002b/py32f002b_hal_msp.c \
    $(FIRMWARE_DIR)/platform/py32f002b/py32f002b_it.c \
    $(SDK_DIR)/Drivers/CMSIS/Device/PY32F0xx/Source/system_py32f0xx.c \
    $(SDK_DIR)/Drivers/PY32F002B_HAL_Driver/Src/py32f002b_hal.c \
    $(SDK_DIR)/Drivers/PY32F002B_HAL_Driver/Src/py32f002b_hal_rcc.c \
    $(SDK_DIR)/Drivers/PY32F002B_HAL_Driver/Src/py32f002b_hal_rcc_ex.c \
    $(SDK_DIR)/Drivers/PY32F002B_HAL_Driver/Src/py32f002b_hal_gpio.c \
    $(SDK_DIR)/Drivers/PY32F002B_HAL_Driver/Src/py32f002b_hal_pwr.c \
    $(SDK_DIR)/Drivers/PY32F002B_HAL_Driver/Src/py32f002b_hal_cortex.c \
    $(SDK_DIR)/Drivers/PY32F002B_HAL_Driver/Src/py32f002b_hal_adc.c \
    $(SDK_DIR)/Drivers/PY32F002B_HAL_Driver/Src/py32f002b_hal_tim.c \
    $(SDK_DIR)/Drivers/PY32F002B_HAL_Driver/Src/py32f002b_hal_iwdg.c

ASM_SOURCES := \
    $(SDK_DIR)/Templates/PY32F002Bxx_Templates/EIDE/startup_py32f002bxx.s

LDSCRIPT := $(SDK_DIR)/Templates/PY32F002Bxx_Templates/EIDE/py32f002bx5.ld

LIBS := --specs=nano.specs --specs=nosys.specs -lm
LDFLAGS := $(CPU) -T$(LDSCRIPT) $(LIBS) -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(C_SOURCES)))
OBJECTS += $(patsubst %.s,$(BUILD_DIR)/%.o,$(notdir $(ASM_SOURCES)))

vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

.PHONY: all clean

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
	$(CC) -c $(CFLAGS) $(C_DEFS) $(C_INCLUDES) $< -o $@

$(BUILD_DIR)/%.o: %.s
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
	$(AS) -c $(ASFLAGS) $(C_DEFS) $(C_INCLUDES) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

clean:
	@if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"

-include $(wildcard $(BUILD_DIR)/*.d)
ifneq ($(filter 1,$(FULL_TEST)),)
ifneq ($(filter 1,$(BENCH_NO_PUMP) $(RADAR_TEST) $(PUMP_TEST) $(BATTERY_TEST) $(BATTERY_PUMP_TEST) $(RELEASE_BUILD)),)
$(error FULL_TEST cannot be combined with other test profiles)
endif
endif
