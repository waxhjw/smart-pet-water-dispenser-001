#ifndef BOARD_PROFILE_H
#define BOARD_PROFILE_H

/*
 * SP01 饮水机 V1.0 原理图引脚表。
 * 来源：SCH_Schematic1_2026-09-17.pdf，后续以 PCB/网表复核为准。
 */

#define BOARD_NAME                  "SP01 pet fountain"
#define BOARD_MCU_ASSUMED           "PY32F002BF15U6TR"

#define BOARD_GPIO_ACTIVE_LOW        0U
#define BOARD_GPIO_ACTIVE_HIGH       1U

/* MCU pins from schematic labels. */
#define BOARD_PIN_REL                "PC1"
#define BOARD_PIN_BUZZER             "PB7"
#define BOARD_PIN_SWDIO              "PB6"
#define BOARD_PIN_CHARGE_DONE        "PB5"
#define BOARD_PIN_CHARGE_ACTIVE      "PB4"
#define BOARD_PIN_LED_RED            "PB3"
#define BOARD_PIN_PUMP_ON            "PB2"
#define BOARD_PIN_ADC_VBAT           "PB1/ADC_IN0"
#define BOARD_PIN_KEY                "PB0"
#define BOARD_PIN_ADC_PUMP           "PA3/ADC_IN1"
#define BOARD_PIN_LED_BLUE           "PA1"
#define BOARD_PIN_SWCLK              "PA2"
#define BOARD_PIN_LED_GREEN          "PA5"
#define BOARD_PIN_NTC_ADC            "PA6/ADC_IN3"
#define BOARD_PIN_REL_EN             "PA7"

/* Electrical conventions that firmware must verify on first board. */
#define BOARD_PUMP_ON_ACTIVE_LEVEL        BOARD_GPIO_ACTIVE_HIGH
#define BOARD_RADAR_ACTIVE_LEVEL          BOARD_GPIO_ACTIVE_HIGH
/*
 * 板上实测：PB0 空闲约为高电平，触摸按住时变低，松手后回高。
 * 因此 MCU 侧必须按低有效解释按键；不要按原理图标签猜成高有效。
 */
#define BOARD_KEY_ACTIVE_LEVEL            BOARD_GPIO_ACTIVE_LOW
#define BOARD_RGB_ACTIVE_LEVEL            BOARD_GPIO_ACTIVE_LOW

/* CHRG/DONE from TP4056-like chargers are often open-drain low-active. */
#define BOARD_CHARGE_ACTIVE_LEVEL         BOARD_GPIO_ACTIVE_LOW
#define BOARD_CHARGE_DONE_LEVEL           BOARD_GPIO_ACTIVE_LOW

/* Hardware notes for bring-up. */
#define BOARD_PUMP_SHUNT_MILLIOHM         330U
/*
 * 电池采样分压：BAT -> 100k -> PB1/ADC_VBAT -> 100k -> GND。
 * 因此 PB1 电压乘 2 才是电池端电压；例如电池 3.4V 时，PB1 约为 1.7V。
 * 该比例以当前原理图和同板实测值为准：R17 电池侧约 3.4V，采样侧约 1.7V。
 */
#define BOARD_VBAT_DIVIDER_TOP_OHM        100000UL
#define BOARD_VBAT_DIVIDER_BOTTOM_OHM     100000UL
#define BOARD_VBAT_DIVIDER_GAIN_NUM       (BOARD_VBAT_DIVIDER_TOP_OHM + \
                                           BOARD_VBAT_DIVIDER_BOTTOM_OHM)
#define BOARD_VBAT_DIVIDER_GAIN_DEN       BOARD_VBAT_DIVIDER_BOTTOM_OHM

#endif
