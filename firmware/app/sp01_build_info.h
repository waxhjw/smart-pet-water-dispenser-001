#ifndef SP01_BUILD_INFO_H
#define SP01_BUILD_INFO_H

#include "app_types.h"

#if defined(SP01_RGB_TUNER_TEST) && (defined(SP01_RELEASE_BUILD) || \
                                    defined(SP01_BENCH_NO_PUMP) || \
                                    defined(SP01_TOUCH_DIAG_TEST) || \
                                    defined(SP01_FULL_TEST) || \
                                    defined(SP01_SPEC_TEST) || \
                                    defined(SP01_LOW_WATER_TEST) || \
                                    defined(SP01_RADAR_TEST) || \
                                    defined(SP01_PUMP_TEST) || \
                                    defined(SP01_BATTERY_TEST) || \
                                    defined(SP01_BATTERY_PUMP_TEST) || \
                                    defined(SP01_PUMP_ADC_TEST) || \
                                    defined(SP01_PUMP_ADC_HOLD_TEST) || \
                                    defined(SP01_RGB_WHITE_BALANCE_TEST) || \
                                    defined(SP01_RGB_TEST) || \
                                    defined(SP01_RGB_SINGLE_TEST) || \
                                    defined(SP01_RGB_ISOLATED_PA5_TEST) || \
                                    defined(SP01_RGB_ISOLATED_PB3_TEST) || \
                                    defined(SP01_RGB_COMBO_PA5_PA1_TEST) || \
                                    defined(SP01_RGB_COMBO_SCAN_TEST) || \
                                    defined(SP01_RGB_TRISTATE) || \
                                    defined(SP01_RGB_ALL_ON_TEST))
#error "SP01_RGB_TUNER_TEST cannot be combined with other build profiles."
#endif

#if defined(SP01_BENCH_NO_PUMP) && defined(SP01_RELEASE_BUILD)
#error "SP01_BENCH_NO_PUMP cannot be combined with SP01_RELEASE_BUILD."
#endif

#if defined(SP01_RGB_WHITE_BALANCE_TEST) && (defined(SP01_RELEASE_BUILD) || \
                                            defined(SP01_BENCH_NO_PUMP) || \
                                            defined(SP01_TOUCH_DIAG_TEST) || \
                                            defined(SP01_FULL_TEST) || \
                                            defined(SP01_SPEC_TEST) || \
                                            defined(SP01_LOW_WATER_TEST) || \
                                            defined(SP01_RADAR_TEST) || \
                                            defined(SP01_PUMP_TEST) || \
                                            defined(SP01_BATTERY_TEST) || \
                                            defined(SP01_BATTERY_PUMP_TEST) || \
                                            defined(SP01_PUMP_ADC_TEST) || \
                                            defined(SP01_PUMP_ADC_HOLD_TEST) || \
                                            defined(SP01_RGB_TEST) || \
                                            defined(SP01_RGB_SINGLE_TEST) || \
                                            defined(SP01_RGB_ISOLATED_PA5_TEST) || \
                                            defined(SP01_RGB_ISOLATED_PB3_TEST) || \
                                            defined(SP01_RGB_COMBO_PA5_PA1_TEST) || \
                                            defined(SP01_RGB_COMBO_SCAN_TEST) || \
                                            defined(SP01_RGB_TRISTATE) || \
                                            defined(SP01_RGB_ALL_ON_TEST))
#error "SP01_RGB_WHITE_BALANCE_TEST cannot be combined with other build profiles."
#endif

#if defined(SP01_TOUCH_DIAG_TEST) && (defined(SP01_RELEASE_BUILD) || \
                                     defined(SP01_BENCH_NO_PUMP) || \
                                     defined(SP01_FULL_TEST) || \
                                     defined(SP01_SPEC_TEST) || \
                                     defined(SP01_LOW_WATER_TEST) || \
                                     defined(SP01_RADAR_TEST) || \
                                     defined(SP01_PUMP_TEST) || \
                                     defined(SP01_BATTERY_TEST) || \
                                     defined(SP01_BATTERY_PUMP_TEST) || \
                                     defined(SP01_PUMP_ADC_TEST) || \
                                     defined(SP01_PUMP_ADC_HOLD_TEST) || \
                                     defined(SP01_RGB_TEST) || \
                                     defined(SP01_RGB_SINGLE_TEST) || \
                                     defined(SP01_RGB_ISOLATED_PA5_TEST) || \
                                     defined(SP01_RGB_ISOLATED_PB3_TEST) || \
                                     defined(SP01_RGB_COMBO_PA5_PA1_TEST) || \
                                     defined(SP01_RGB_COMBO_SCAN_TEST) || \
                                     defined(SP01_RGB_TRISTATE) || \
                                     defined(SP01_RGB_ALL_ON_TEST))
#error "SP01_TOUCH_DIAG_TEST cannot be combined with other build profiles."
#endif

#if defined(SP01_FULL_TEST) && (defined(SP01_BENCH_NO_PUMP) || \
                                defined(SP01_RADAR_TEST) || \
                                defined(SP01_PUMP_TEST) || \
                                defined(SP01_BATTERY_TEST) || \
                                defined(SP01_BATTERY_PUMP_TEST) || \
                                defined(SP01_RELEASE_BUILD))
#error "SP01_FULL_TEST cannot be combined with other build profiles."
#endif

#if defined(SP01_SPEC_TEST) && (defined(SP01_RELEASE_BUILD) || \
                                defined(SP01_FULL_TEST) || \
                                defined(SP01_LOW_WATER_TEST) || \
                                defined(SP01_PUMP_TEST) || \
                                defined(SP01_BATTERY_TEST) || \
                                defined(SP01_BATTERY_PUMP_TEST) || \
                                defined(SP01_RGB_TEST) || \
                                defined(SP01_RGB_SINGLE_TEST) || \
                                defined(SP01_RGB_ISOLATED_PA5_TEST) || \
                                defined(SP01_RGB_ISOLATED_PB3_TEST) || \
                                defined(SP01_RGB_COMBO_PA5_PA1_TEST) || \
                                defined(SP01_RGB_COMBO_SCAN_TEST) || \
                                defined(SP01_RGB_ALL_ON_TEST))
#error "SP01_SPEC_TEST cannot be combined with other build profiles."
#endif

#if (defined(SP01_SPEC_TEST002) || defined(SP01_SPEC_TEST003) || \
     defined(SP01_SPEC_TEST004) || defined(SP01_SPEC_TEST005) || \
     defined(SP01_SPEC_TEST006) || defined(SP01_SPEC_TEST007)) && \
    !defined(SP01_SPEC_TEST)
#error "SP01_SPEC_TEST002/003/004/005/006/007 must use the specification test state machine."
#endif

#if defined(SP01_SPEC_TEST005) && (defined(SP01_RELEASE_BUILD) || \
                                   defined(SP01_FULL_TEST) || \
                                   defined(SP01_LOW_WATER_TEST) || \
                                   defined(SP01_PUMP_TEST) || \
                                   defined(SP01_BATTERY_TEST) || \
                                   defined(SP01_BATTERY_PUMP_TEST) || \
                                   defined(SP01_PUMP_ADC_TEST) || \
                                   defined(SP01_PUMP_ADC_HOLD_TEST) || \
                                   defined(SP01_RGB_TEST) || \
                                   defined(SP01_RGB_SINGLE_TEST) || \
                                   defined(SP01_RGB_ISOLATED_PA5_TEST) || \
                                   defined(SP01_RGB_ISOLATED_PB3_TEST) || \
                                   defined(SP01_RGB_COMBO_PA5_PA1_TEST) || \
                                   defined(SP01_RGB_COMBO_SCAN_TEST) || \
                                   defined(SP01_RGB_ALL_ON_TEST))
#error "SP01_SPEC_TEST005 cannot be combined with other build profiles."
#endif

#if defined(SP01_SPEC_TEST006) && (defined(SP01_RELEASE_BUILD) || \
                                   defined(SP01_FULL_TEST) || \
                                   defined(SP01_LOW_WATER_TEST) || \
                                   defined(SP01_PUMP_TEST) || \
                                   defined(SP01_BATTERY_TEST) || \
                                   defined(SP01_BATTERY_PUMP_TEST) || \
                                   defined(SP01_PUMP_ADC_TEST) || \
                                   defined(SP01_PUMP_ADC_HOLD_TEST) || \
                                   defined(SP01_RGB_TEST) || \
                                   defined(SP01_RGB_SINGLE_TEST) || \
                                   defined(SP01_RGB_ISOLATED_PA5_TEST) || \
                                   defined(SP01_RGB_ISOLATED_PB3_TEST) || \
                                   defined(SP01_RGB_COMBO_PA5_PA1_TEST) || \
                                   defined(SP01_RGB_COMBO_SCAN_TEST) || \
                                   defined(SP01_RGB_ALL_ON_TEST))
#error "SP01_SPEC_TEST006 cannot be combined with other build profiles."
#endif

#if defined(SP01_SPEC_TEST007) && (defined(SP01_RELEASE_BUILD) || \
                                   defined(SP01_FULL_TEST) || \
                                   defined(SP01_LOW_WATER_TEST) || \
                                   defined(SP01_PUMP_TEST) || \
                                   defined(SP01_BATTERY_TEST) || \
                                   defined(SP01_BATTERY_PUMP_TEST) || \
                                   defined(SP01_PUMP_ADC_TEST) || \
                                   defined(SP01_PUMP_ADC_HOLD_TEST) || \
                                   defined(SP01_RGB_TEST) || \
                                   defined(SP01_RGB_SINGLE_TEST) || \
                                   defined(SP01_RGB_ISOLATED_PA5_TEST) || \
                                   defined(SP01_RGB_ISOLATED_PB3_TEST) || \
                                   defined(SP01_RGB_COMBO_PA5_PA1_TEST) || \
                                   defined(SP01_RGB_COMBO_SCAN_TEST) || \
                                   defined(SP01_RGB_ALL_ON_TEST))
#error "SP01_SPEC_TEST007 cannot be combined with other build profiles."
#endif

#if defined(SP01_LOW_WATER_TEST) && (!defined(SP01_FULL_TEST) || \
                                     defined(SP01_RELEASE_BUILD) || \
                                     defined(SP01_BENCH_NO_PUMP) || \
                                     defined(SP01_RADAR_TEST) || \
                                     defined(SP01_PUMP_TEST) || \
                                     defined(SP01_BATTERY_TEST) || \
                                     defined(SP01_BATTERY_PUMP_TEST))
#error "SP01_LOW_WATER_TEST requires the Type-C full-test path only."
#endif

#if defined(SP01_PUMP_ADC_TEST) && (defined(SP01_RELEASE_BUILD) || \
                                    defined(SP01_FULL_TEST) || \
                                    defined(SP01_LOW_WATER_TEST) || \
                                    defined(SP01_PUMP_ADC_HOLD_TEST) || \
                                    defined(SP01_BENCH_NO_PUMP) || \
                                    defined(SP01_RADAR_TEST) || \
                                    defined(SP01_PUMP_TEST) || \
                                    defined(SP01_BATTERY_TEST) || \
                                    defined(SP01_BATTERY_PUMP_TEST))
#error "SP01_PUMP_ADC_TEST cannot be combined with other build profiles."
#endif

#if defined(SP01_PUMP_ADC_HOLD_TEST) && (defined(SP01_RELEASE_BUILD) || \
                                         defined(SP01_FULL_TEST) || \
                                         defined(SP01_LOW_WATER_TEST) || \
                                         defined(SP01_PUMP_ADC_TEST) || \
                                         defined(SP01_BENCH_NO_PUMP) || \
                                         defined(SP01_RADAR_TEST) || \
                                         defined(SP01_PUMP_TEST) || \
                                         defined(SP01_BATTERY_TEST) || \
                                         defined(SP01_BATTERY_PUMP_TEST) || \
                                         defined(SP01_RGB_TEST) || \
                                         defined(SP01_RGB_SINGLE_TEST) || \
                                         defined(SP01_RGB_ISOLATED_PA5_TEST) || \
                                         defined(SP01_RGB_ISOLATED_PB3_TEST) || \
                                         defined(SP01_RGB_COMBO_PA5_PA1_TEST) || \
                                         defined(SP01_RGB_COMBO_SCAN_TEST))
#error "SP01_PUMP_ADC_HOLD_TEST cannot be combined with other build profiles."
#endif

#if defined(SP01_PUMP_TEST) && (defined(SP01_BENCH_NO_PUMP) || \
                                defined(SP01_RADAR_TEST) || \
                                defined(SP01_BATTERY_TEST) || \
                                defined(SP01_BATTERY_PUMP_TEST) || \
                                defined(SP01_RELEASE_BUILD))
#error "SP01_PUMP_TEST cannot be combined with other build profiles."
#endif

#if defined(SP01_BATTERY_TEST) && (defined(SP01_BENCH_NO_PUMP) || \
                                   defined(SP01_RADAR_TEST) || \
                                   defined(SP01_BATTERY_PUMP_TEST) || \
                                   defined(SP01_RELEASE_BUILD))
#error "SP01_BATTERY_TEST cannot be combined with other build profiles."
#endif

#if defined(SP01_BATTERY_PUMP_TEST) && (defined(SP01_BENCH_NO_PUMP) || \
                                        defined(SP01_RADAR_TEST) || \
                                        defined(SP01_BATTERY_TEST) || \
                                        defined(SP01_RELEASE_BUILD))
#error "SP01_BATTERY_PUMP_TEST cannot be combined with other build profiles."
#endif

#if defined(SP01_RGB_SINGLE_TEST) && (defined(SP01_RGB_TEST) || \
                                     defined(SP01_RGB_ISOLATED_PA5_TEST) || \
                                     defined(SP01_FULL_TEST) || \
                                     defined(SP01_BENCH_NO_PUMP) || \
                                     defined(SP01_RADAR_TEST) || \
                                     defined(SP01_PUMP_TEST) || \
                                     defined(SP01_BATTERY_TEST) || \
                                     defined(SP01_BATTERY_PUMP_TEST) || \
                                     defined(SP01_RELEASE_BUILD))
#error "SP01_RGB_SINGLE_TEST cannot be combined with other build profiles."
#endif

#if defined(SP01_RGB_ISOLATED_PA5_TEST) && (defined(SP01_RGB_TEST) || \
                                           defined(SP01_RGB_SINGLE_TEST) || \
                                           defined(SP01_FULL_TEST) || \
                                           defined(SP01_BENCH_NO_PUMP) || \
                                           defined(SP01_RADAR_TEST) || \
                                           defined(SP01_PUMP_TEST) || \
                                           defined(SP01_BATTERY_TEST) || \
                                           defined(SP01_BATTERY_PUMP_TEST) || \
                                           defined(SP01_RELEASE_BUILD))
#error "SP01_RGB_ISOLATED_PA5_TEST cannot be combined with other build profiles."
#endif

#if defined(SP01_RGB_ISOLATED_PB3_TEST) && (defined(SP01_RGB_TEST) || \
                                           defined(SP01_RGB_SINGLE_TEST) || \
                                           defined(SP01_RGB_ISOLATED_PA5_TEST) || \
                                           defined(SP01_FULL_TEST) || \
                                           defined(SP01_BENCH_NO_PUMP) || \
                                           defined(SP01_RADAR_TEST) || \
                                           defined(SP01_PUMP_TEST) || \
                                           defined(SP01_BATTERY_TEST) || \
                                           defined(SP01_BATTERY_PUMP_TEST) || \
                                           defined(SP01_RELEASE_BUILD))
#error "SP01_RGB_ISOLATED_PB3_TEST cannot be combined with other build profiles."
#endif

#if defined(SP01_RGB_COMBO_PA5_PA1_TEST) && (defined(SP01_RGB_TEST) || \
                                            defined(SP01_RGB_SINGLE_TEST) || \
                                            defined(SP01_RGB_ISOLATED_PA5_TEST) || \
                                            defined(SP01_RGB_ISOLATED_PB3_TEST) || \
                                            defined(SP01_FULL_TEST) || \
                                            defined(SP01_BENCH_NO_PUMP) || \
                                            defined(SP01_RADAR_TEST) || \
                                            defined(SP01_PUMP_TEST) || \
                                            defined(SP01_BATTERY_TEST) || \
                                            defined(SP01_BATTERY_PUMP_TEST) || \
                                            defined(SP01_RELEASE_BUILD))
#error "SP01_RGB_COMBO_PA5_PA1_TEST cannot be combined with other build profiles."
#endif

#if defined(SP01_RGB_COMBO_SCAN_TEST) && (defined(SP01_RGB_TEST) || \
                                         defined(SP01_RGB_SINGLE_TEST) || \
                                         defined(SP01_RGB_ISOLATED_PA5_TEST) || \
                                         defined(SP01_RGB_ISOLATED_PB3_TEST) || \
                                         defined(SP01_RGB_COMBO_PA5_PA1_TEST) || \
                                         defined(SP01_FULL_TEST) || \
                                         defined(SP01_BENCH_NO_PUMP) || \
                                         defined(SP01_RADAR_TEST) || \
                                         defined(SP01_PUMP_TEST) || \
                                         defined(SP01_BATTERY_TEST) || \
                                         defined(SP01_BATTERY_PUMP_TEST) || \
                                         defined(SP01_RELEASE_BUILD))
#error "SP01_RGB_COMBO_SCAN_TEST cannot be combined with other build profiles."
#endif

#if defined(SP01_RGB_ALL_ON_TEST) && (defined(SP01_RELEASE_BUILD) || \
                                      defined(SP01_FULL_TEST) || \
                                      defined(SP01_LOW_WATER_TEST) || \
                                      defined(SP01_PUMP_ADC_TEST) || \
                                      defined(SP01_PUMP_ADC_HOLD_TEST) || \
                                      defined(SP01_BENCH_NO_PUMP) || \
                                      defined(SP01_RADAR_TEST) || \
                                      defined(SP01_PUMP_TEST) || \
                                      defined(SP01_BATTERY_TEST) || \
                                      defined(SP01_BATTERY_PUMP_TEST) || \
                                      defined(SP01_RGB_TEST) || \
                                      defined(SP01_RGB_SINGLE_TEST) || \
                                      defined(SP01_RGB_ISOLATED_PA5_TEST) || \
                                      defined(SP01_RGB_ISOLATED_PB3_TEST) || \
                                      defined(SP01_RGB_COMBO_PA5_PA1_TEST) || \
                                      defined(SP01_RGB_COMBO_SCAN_TEST))
#error "SP01_RGB_ALL_ON_TEST cannot be combined with other build profiles."
#endif

#if defined(SP01_RGB_TEST) && (defined(SP01_RGB_SINGLE_TEST) || \
                               defined(SP01_RELEASE_BUILD) || \
                               defined(SP01_FULL_TEST) || \
                               defined(SP01_BENCH_NO_PUMP) || \
                               defined(SP01_RADAR_TEST) || \
                               defined(SP01_PUMP_TEST) || \
                               defined(SP01_BATTERY_TEST) || \
                               defined(SP01_BATTERY_PUMP_TEST))
#error "SP01_RGB_TEST cannot be combined with other build profiles."
#endif

#if defined(SP01_RADAR_TEST) && !defined(SP01_BENCH_NO_PUMP)
#error "SP01_RADAR_TEST requires SP01_BENCH_NO_PUMP."
#endif

/*
 * 固件版本统一入口。
 * 版本号先按 0.1.0 首板验证版管理，后续每次改变产品行为时递增。
 */
#define SP01_FW_VERSION_MAJOR       0U
#define SP01_FW_VERSION_MINOR       1U
#define SP01_FW_VERSION_PATCH       0U
#define SP01_FW_VERSION_CODE        ((app_u32)(SP01_FW_VERSION_MAJOR * 10000U + \
                                               SP01_FW_VERSION_MINOR * 100U + \
                                               SP01_FW_VERSION_PATCH))

/* 未定义 SP01_RELEASE_BUILD 时，默认保留首板调试观察点。 */
#ifdef SP01_RELEASE_BUILD
#define SP01_BUILD_PROFILE_RELEASE  1U
#define SP01_BUILD_PROFILE_NAME     "RELEASE"
#elif defined(SP01_TOUCH_DIAG_TEST)
#define SP01_BUILD_PROFILE_TOUCH_DIAG_TEST  22U
#define SP01_BUILD_PROFILE_NAME              "TOUCH_DIAG_TEST001"
#elif defined(SP01_LOW_WATER_TEST)
#define SP01_BUILD_PROFILE_LOW_WATER_TEST  14U
#define SP01_BUILD_PROFILE_NAME             "LOW_WATER_TEST"
#elif defined(SP01_PUMP_ADC_TEST)
#define SP01_BUILD_PROFILE_PUMP_ADC_TEST  15U
#define SP01_BUILD_PROFILE_NAME            "PUMP_ADC_TEST"
#elif defined(SP01_PUMP_ADC_HOLD_TEST)
#define SP01_BUILD_PROFILE_PUMP_ADC_HOLD_TEST  16U
#define SP01_BUILD_PROFILE_NAME                 "PUMP_ADC_HOLD_TEST"
#elif defined(SP01_FULL_TEST)
#define SP01_BUILD_PROFILE_FULL_TEST  7U
#define SP01_BUILD_PROFILE_NAME       "FULL_TEST"
#elif defined(SP01_SPEC_TEST003)
#define SP01_BUILD_PROFILE_SPEC_TEST003  20U
#define SP01_BUILD_PROFILE_NAME          "SPEC_TEST003"
#elif defined(SP01_SPEC_TEST004)
#define SP01_BUILD_PROFILE_SPEC_TEST004  21U
#define SP01_BUILD_PROFILE_NAME          "SPEC_TEST004"
#elif defined(SP01_SPEC_TEST005)
#define SP01_BUILD_PROFILE_SPEC_TEST005  23U
#define SP01_BUILD_PROFILE_NAME          "SPEC_TEST005_LOW_WATER"
#elif defined(SP01_SPEC_TEST006)
#define SP01_BUILD_PROFILE_SPEC_TEST006  24U
#define SP01_BUILD_PROFILE_NAME          "SPEC_TEST006_LOW_WATER_RECOVERY"
#elif defined(SP01_SPEC_TEST007)
#define SP01_BUILD_PROFILE_SPEC_TEST007  25U
#define SP01_BUILD_PROFILE_NAME          "SPEC_TEST007_LOW_WATER_WARNING"
#elif defined(SP01_SPEC_TEST002)
#define SP01_BUILD_PROFILE_SPEC_TEST002  19U
#define SP01_BUILD_PROFILE_NAME          "SPEC_TEST002"
#elif defined(SP01_SPEC_TEST)
#define SP01_BUILD_PROFILE_SPEC_TEST  18U
#define SP01_BUILD_PROFILE_NAME       "SPEC_TEST"
#elif defined(SP01_RGB_TEST)
#define SP01_BUILD_PROFILE_RGB_TEST  8U
#define SP01_BUILD_PROFILE_NAME      "RGB_TEST"
#elif defined(SP01_RGB_SINGLE_TEST)
#define SP01_BUILD_PROFILE_RGB_SINGLE_TEST  9U
#define SP01_BUILD_PROFILE_NAME             "RGB_SINGLE_TEST"
#elif defined(SP01_RGB_ISOLATED_PA5_TEST)
#define SP01_BUILD_PROFILE_RGB_ISOLATED_PA5_TEST  10U
#define SP01_BUILD_PROFILE_NAME                    "RGB_ISOLATED_PA5_TEST"
#elif defined(SP01_RGB_ISOLATED_PB3_TEST)
#define SP01_BUILD_PROFILE_RGB_ISOLATED_PB3_TEST  11U
#define SP01_BUILD_PROFILE_NAME                    "RGB_ISOLATED_PB3_TEST"
#elif defined(SP01_RGB_COMBO_PA5_PA1_TEST)
#define SP01_BUILD_PROFILE_RGB_COMBO_PA5_PA1_TEST  12U
#define SP01_BUILD_PROFILE_NAME                    "RGB_COMBO_PA5_PA1_TEST"
#elif defined(SP01_RGB_COMBO_SCAN_TEST)
#define SP01_BUILD_PROFILE_RGB_COMBO_SCAN_TEST  13U
#define SP01_BUILD_PROFILE_NAME                  "RGB_COMBO_SCAN_TEST"
#elif defined(SP01_RGB_ALL_ON_TEST)
#define SP01_BUILD_PROFILE_RGB_ALL_ON_TEST  17U
#define SP01_BUILD_PROFILE_NAME             "RGB_ALL_ON_TEST"
#elif defined(SP01_RGB_WHITE_BALANCE_TEST)
#define SP01_BUILD_PROFILE_RGB_WHITE_BALANCE_TEST  26U
#define SP01_BUILD_PROFILE_NAME                    "RGB_WHITE_BALANCE_TEST001"
#elif defined(SP01_RGB_TUNER_TEST)
#define SP01_BUILD_PROFILE_RGB_TUNER_TEST  27U
#define SP01_BUILD_PROFILE_NAME            "RGB_TUNER_TEST001"
#elif defined(SP01_RADAR_TEST)
#define SP01_BUILD_PROFILE_RADAR_TEST  3U
#define SP01_BUILD_PROFILE_NAME        "RADAR_TEST"
#elif defined(SP01_PUMP_TEST)
#define SP01_BUILD_PROFILE_PUMP_TEST  4U
#define SP01_BUILD_PROFILE_NAME       "PUMP_TEST"
#elif defined(SP01_BATTERY_TEST)
#define SP01_BUILD_PROFILE_BATTERY_TEST  5U
#define SP01_BUILD_PROFILE_NAME           "BATTERY_TEST"
#elif defined(SP01_BATTERY_PUMP_TEST)
#define SP01_BUILD_PROFILE_BATTERY_PUMP_TEST  6U
#define SP01_BUILD_PROFILE_NAME               "BATTERY_PUMP_TEST"
#elif defined(SP01_BENCH_NO_PUMP)
#define SP01_BUILD_PROFILE_BENCH_NO_PUMP  2U
#define SP01_BUILD_PROFILE_NAME           "BENCH_NO_PUMP"
#else
#define SP01_BUILD_PROFILE_DEBUG    1U
#define SP01_BUILD_PROFILE_NAME     "BRINGUP"
#endif

#endif
