#!/usr/bin/env python3
"""SP01 firmware source checks and host-side unit tests."""

from __future__ import annotations

import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parent
BUILD = ROOT / "build" / "host-tests"


def read(rel: str) -> str:
    return (ROOT / rel).read_text(encoding="utf-8")


def read_repo(rel: str) -> str:
    return (REPO / rel).read_text(encoding="utf-8")


def project_sources(suffix: str):
    """Return only SP01-owned sources; vendor SDK is checked by its own release."""
    vendor_root = ROOT / "vendor"
    return [
        path
        for path in ROOT.rglob(suffix)
        if vendor_root not in path.parents
    ]


def test_no_old_dialogue_project_names() -> None:
    forbidden = ("dialogue_app", "AI_START", "STC8H8K64U", "TARGET_STC8")
    for path in project_sources("*.c"):
        text = path.read_text(encoding="utf-8")
        for word in forbidden:
            assert word not in text, f"old project token {word!r} remains in {path}"
    for path in project_sources("*.h"):
        text = path.read_text(encoding="utf-8")
        for word in forbidden:
            assert word not in text, f"old project token {word!r} remains in {path}"


def test_sp01_state_machine_contract_is_present() -> None:
    header = read("app/sp01_app.h")
    touch_header = read("app/sp01_touch_button.h")
    for name in (
        "SP01_STATE_BOOT",
        "SP01_STATE_STANDBY",
        "SP01_STATE_SENSOR_RUN",
        "SP01_STATE_LOCKOUT",
        "SP01_STATE_LOW_WATER_ALARM",
        "SP01_PUMP_DIAG_LOW_WATER_SUSPECT",
    ):
        assert name in header
    assert "sp01_touch_button_t key_button" in header
    assert "sp01_touch_button_update" in touch_header


def test_build_info_is_present() -> None:
    build_info = read("app/sp01_build_info.h")
    main = read("main.c")
    assert "SP01_FW_VERSION_CODE" in build_info
    assert "SP01_BUILD_PROFILE_NAME" in build_info
    assert "g_sp01_debug_fw_version" in main
    assert "g_sp01_debug_build_profile" in main
    assert "#ifndef SP01_RELEASE_BUILD" in main


def test_radar_test_profile_beeps_on_debounced_rising_edge() -> None:
    build_info = read("app/sp01_build_info.h")
    main = read("main.c")
    makefile = read("target/py32f002b/Makefile")
    build_script = read_repo("tools/build-sp01-firmware.ps1")

    assert "SP01_RADAR_TEST requires SP01_BENCH_NO_PUMP" in build_info
    assert "SP01_BUILD_PROFILE_RADAR_TEST" in main
    assert "g_radar_test_elapsed_ms < 5000UL" in main
    assert "g_radar_test_idle_ms >= 2000UL" in main
    assert "radar_event.active == APP_FALSE" in main
    assert "outputs.radar_power_enable = APP_TRUE" in main
    assert "outputs.pump_enable = APP_FALSE" in main
    assert "outputs.buzzer_mode = SP01_BUZZER_RADAR_TEST" in main
    assert "SP01_BUZZER_RADAR_TEST_MS           250UL" in read("app/app_config.h")
    assert "C_DEFS += -DSP01_RADAR_TEST -DSP01_BENCH_NO_PUMP" in makefile
    assert "[switch]$RadarTest" in build_script
    assert '"RADAR_TEST=1"' in build_script


def test_pump_test_profile_is_continuous_and_separate() -> None:
    build_info = read("app/sp01_build_info.h")
    config = read("app/app_config.h")
    main = read("main.c")
    makefile = read("target/py32f002b/Makefile")
    build_script = read_repo("tools/build-sp01-firmware.ps1")

    assert "SP01_BUILD_PROFILE_PUMP_TEST" in main
    assert "SP01_PUMP_TEST_RELEASE_ARM_MS       500UL" in config
    assert "SP01_PUMP_TEST_LONG_PRESS_MS       2000UL" in config
    assert "SP01_PUMP_TEST_RUN_MS              2000UL" in config
    assert "SP01_PUMP_TEST cannot be combined with other build profiles" in build_info
    assert "g_last_outputs = sp01_pump_test_outputs();" in main
    assert "outputs = sp01_pump_test_outputs();" in main
    assert "sp01_pump_test_update" not in main
    assert "g_pump_power_on_elapsed_ms" not in main
    assert "outputs.pump_enable = APP_TRUE" in main
    assert "outputs.radar_power_enable = APP_FALSE" in main
    board_port = read("platform/py32f002b/board_port.c")
    assert "#if defined(SP01_PUMP_TEST) || defined(SP01_BATTERY_PUMP_TEST)" in board_port
    assert "board_output_level(APP_TRUE, BOARD_PUMP_ON_ACTIVE_LEVEL)" in board_port
    assert "#define BOARD_PUMP_ON_ACTIVE_LEVEL        BOARD_GPIO_ACTIVE_HIGH" in read("app/board_profile.h")
    assert "case BOARD_VENDOR_PIN_PUMP_ON:\n    case BOARD_VENDOR_PIN_KEY:" in read("platform/py32f002b/vendor_port.c")
    assert "case BOARD_VENDOR_PIN_PUMP_ON:\n        return GPIO_PIN_2;" in read("platform/py32f002b/vendor_port.c")
    assert "C_DEFS += -DSP01_PUMP_TEST" in makefile
    assert "$(FIRMWARE_DIR)/app/sp01_pump_test.c" in makefile
    assert '[switch]$PumpTest' in build_script
    assert '"PUMP_TEST=1"' in build_script


def test_battery_test_profile_is_safe_and_observable() -> None:
    build_info = read("app/sp01_build_info.h")
    main = read("main.c")
    battery_header = read("app/sp01_battery_test.h")
    battery_source = read("app/sp01_battery_test.c")
    makefile = read("target/py32f002b/Makefile")
    build_script = read_repo("tools/build-sp01-firmware.ps1")

    assert "SP01_BUILD_PROFILE_BATTERY_TEST" in main
    assert "SP01_BATTERY_TEST cannot be combined with other build profiles" in build_info
    assert "sp01_battery_test_led_mode" in battery_header
    assert "charge_status->conflict" in battery_source
    assert "outputs.pump_enable = APP_FALSE" in main
    assert "outputs.radar_power_enable = APP_FALSE" in main
    assert "C_DEFS += -DSP01_BATTERY_TEST" in makefile
    assert "sp01_battery_test.c" in makefile
    assert '[switch]$BatteryTest' in build_script
    assert '"BATTERY_TEST=1"' in build_script


def test_battery_pump_profile_is_always_on() -> None:
    build_info = read("app/sp01_build_info.h")
    main = read("main.c")
    test_header = read("app/sp01_battery_pump_test.h")
    test_source = read("app/sp01_battery_pump_test.c")
    makefile = read("target/py32f002b/Makefile")
    build_script = read_repo("tools/build-sp01-firmware.ps1")
    tasks = json.loads(read_repo(".vscode/tasks.json"))

    assert "SP01_BUILD_PROFILE_BATTERY_PUMP_TEST" in build_info
    assert "sp01_battery_pump_test_update(sp01_battery_pump_test_t *test)" in test_header
    assert "test->pump_on = APP_TRUE" in test_source
    assert "return APP_TRUE" in test_source
    assert "持续高电平" in main
    assert "SP01_BATTERY_PUMP_TEST" in main
    assert "C_DEFS += -DSP01_BATTERY_PUMP_TEST" in makefile
    assert "sp01_battery_pump_test.c" in makefile
    assert '[switch]$BatteryPumpTest' in build_script
    assert '"BATTERY_PUMP_TEST=1"' in build_script
    labels = {task["label"] for task in tasks["tasks"]}
    assert "SP01: flash battery pump cycle test" in labels


def test_full_test_profile_is_type_c_only() -> None:
    build_info = read("app/sp01_build_info.h")
    full_header = read("app/sp01_full_test.h")
    full_source = read("app/sp01_full_test.c")
    main = read("main.c")
    makefile = read("target/py32f002b/Makefile")
    build_script = read_repo("tools/build-sp01-firmware.ps1")
    verify_script = read_repo("tools/verify-sp01-artifacts.ps1")
    tasks = json.loads(read_repo(".vscode/tasks.json"))

    assert "SP01_FULL_TEST cannot be combined" in build_info
    assert "SP01_BUILD_PROFILE_FULL_TEST" in main
    assert "sp01_full_test_tick" in main
    assert "SP01_FULL_TEST" in makefile
    assert "sp01_full_test.c" in makefile
    assert "[switch]$FullTest001" in build_script
    assert '[Alias("FullTest")]' in build_script
    assert '"FULL_TEST=1"' in build_script
    assert "[switch]$FullTest001" in verify_script
    assert '[Alias("FullTest")]' in verify_script
    assert '"build-full-test001"' in verify_script
    assert "SP01_FULL_POWER_ON_CONTINUOUS" in full_header
    assert "SP01_FULL_TEST_RADAR_COOLDOWN_MS" in full_source
    labels = {task["label"] for task in tasks["tasks"]}
    assert "SP01: full test001 build" in labels
    assert "SP01: flash full test001" in labels
    assert "SP01: verify full test001 artifacts" in labels
    assert "build-full-test001" in tasks["tasks"][0].get("label", "") or "build-full-test001" in read_repo(".vscode/tasks.json")


def test_spec_product_profile_is_supply_aware() -> None:
    build_info = read("app/sp01_build_info.h")
    spec_header = read("app/sp01_spec_product.h")
    spec_source = read("app/sp01_spec_product.c")
    main = read("main.c")
    board_port = read("platform/py32f002b/board_port.c")
    vendor_port = read("platform/py32f002b/vendor_port.c")
    interrupt = read("platform/py32f002b/py32f002b_it.c")
    makefile = read("target/py32f002b/Makefile")
    build_script = read_repo("tools/build-sp01-firmware.ps1")
    verify_script = read_repo("tools/verify-sp01-artifacts.ps1")
    tasks = json.loads(read_repo(".vscode/tasks.json"))

    assert "SP01_SPEC_TEST" in build_info
    assert "SP01_SPEC_SUPPLY_TYPEC" in spec_header
    assert "SP01_SPEC_SUPPLY_BATTERY" in spec_header
    assert "sp01_spec_detect_supply" in spec_source
    assert "CHRG 或 DONE 有效" in spec_source
    assert "SP01_SPEC_STATE_TYPEC_CONTINUOUS" in spec_source
    assert "SP01_SPEC_FUR_FLUSH_INTERVAL_MS" in spec_source
    assert "SP01_SPEC_TEST" in main
    assert "C_DEFS += -DSP01_SPEC_TEST" in makefile
    assert "sp01_spec_product.c" in makefile
    assert "[switch]$SpecTest001" in build_script
    assert '"SPEC_TEST=1"' in build_script
    assert "[switch]$SpecTest001" in verify_script
    assert '"build-spec-test001"' in verify_script
    assert "SP01_SPEC_SUPPLY_DEBOUNCE_TICKS" in read("app/app_config.h")
    assert "SP01_SPEC_TEST002" in read("app/sp01_build_info.h")
    assert "SPEC_TEST002=1" in build_script
    assert "SP01_SPEC_TYPEC_BAT_NODE_MIN_MV" in spec_source
    assert "SPEC_TEST003=1" in build_script
    assert '[switch]$SpecTest003' in verify_script
    assert '"build-spec-test003"' in verify_script
    assert "SPEC_TEST004=1" in build_script
    assert '[switch]$SpecTest004' in verify_script
    assert '"build-spec-test004"' in verify_script
    assert "SP01_SPEC_RADAR_STARTUP_WARMUP_MS" in spec_source
    assert "SP01_SPEC_RADAR_IDLE_ARM_MS" in spec_source
    assert "SP01_SPEC_TEST005" in build_info
    assert "SP01_SPEC_TEST006" in build_info
    assert "SP01_SPEC_TEST007" in build_info
    assert "defined(SP01_RGB_WHITE_BALANCE_TEST) || defined(SP01_SPEC_TEST007)" in board_port
    assert "case SP01_LED_PRODUCT_CONTINUOUS:" in board_port
    assert "case SP01_LED_PRODUCT_CHARGING:" in board_port
    assert "case SP01_LED_PRODUCT_FULL:" in board_port
    assert "case SP01_LED_PRODUCT_LOW_WATER_WARNING_TYPEC:" in board_port
    assert "red_duty = 100U;" in board_port
    assert "green_duty = 60U;" in board_port
    assert "blue_duty = 85U;" in board_port
    assert "board_vendor_rgb_pwm_apply(red_duty, green_duty, blue_duty);" in board_port
    assert "sp01_indicator_render(led_mode, buzzer_mode, &buzzer_pins);" in board_port
    assert "board_vendor_buzzer_apply(buzzer_pins.buzzer_on,\n                                  buzzer_frequency_hz,\n                                  buzzer_duty_percent);" in board_port
    assert "/* 白平衡调色固件没有触摸业务，保持静音。 */\n    board_vendor_buzzer_apply(APP_FALSE, 0U, 0U);" in board_port
    assert "defined(SP01_RGB_WHITE_BALANCE_TEST) || defined(SP01_SPEC_TEST007)" in vendor_port
    assert "TIM1_BRK_UP_TRG_COM_IRQHandler" in interrupt
    assert "board_vendor_rgb_pwm_tim1_irq" in interrupt
    assert "SP01_RGB_TUNER_TIMER_TICK_HZ" in vendor_port
    assert "SP01_RGB_TUNER_PWM_STEP_COUNT" in vendor_port
    assert "SP01_SPEC_LOW_WATER_THRESHOLD_MV" in spec_source
    assert "SP01_SPEC_LOW_WATER_RECOVERY_THRESHOLD_MV" in spec_source
    assert "SP01_SPEC_LOW_WATER_WARNING_THRESHOLD_MV" in read("app/app_config.h")
    assert "SP01_LED_PRODUCT_LOW_WATER_WARNING_TYPEC" in spec_source
    assert "SP01_LED_PRODUCT_LOW_WATER_WARNING_BATTERY" in spec_source
    assert "pump_sense_mv = board_inputs.pump_sense_mv" in main
    assert "SPEC_TEST005=1" in build_script
    assert '[switch]$SpecTest005' in verify_script
    assert '"build-spec-test005"' in verify_script
    assert "SPEC_TEST006=1" in build_script
    assert '[switch]$SpecTest006' in verify_script
    assert '"build-spec-test006"' in verify_script
    assert "SPEC_TEST007=1" in build_script
    assert '[switch]$SpecTest007' in verify_script
    assert '"build-spec-test007"' in verify_script
    assert '[switch]$SpecTest007Retry30s' in build_script
    assert '"BUILD_DIR=build-spec-test007-retry30s"' in build_script
    assert '[switch]$SpecTest007Retry30s' in verify_script
    assert '"build-spec-test007-retry30s"' in verify_script
    assert '[switch]$SpecTest002' in verify_script
    assert '"build-spec-test002"' in verify_script
    assert "!defined(SP01_SPEC_TEST)" in main

    labels = {task["label"] for task in tasks["tasks"]}
    assert "SP01: spec test001 build" in labels
    assert "SP01: verify spec test001 artifacts" in labels
    assert "SP01: flash spec test001" in labels
    assert "SP01: spec test002 build" in labels
    assert "SP01: verify spec test002 artifacts" in labels
    assert "SP01: flash spec test002" in labels
    assert "SP01: spec test003 build" in labels
    assert "SP01: verify spec test003 artifacts" in labels
    assert "SP01: flash spec test003" in labels
    assert "SP01: spec test004 build" in labels
    assert "SP01: verify spec test004 artifacts" in labels
    assert "SP01: flash spec test004" in labels
    assert "SP01: spec test006 build" in labels
    assert "SP01: verify spec test006 artifacts" in labels
    assert "SP01: flash spec test006" in labels
    assert "SP01: spec test007 build" in labels
    assert "SP01: verify spec test007 artifacts" in labels
    assert "SP01: flash spec test007" in labels
    assert "build-spec-test007-retry30s" in read_repo(".vscode/tasks.json")
    assert "SP01_TOUCH_DIAG_TEST cannot be combined with other build profiles" in read("app/sp01_build_info.h")
    assert "TOUCH_DIAG_TEST=1" in build_script
    assert '[switch]$TouchDiagTest001' in verify_script
    assert '"build-touch-diag-test001"' in verify_script
    assert "SP01: touch diagnostic test001 build" in labels
    assert "SP01: verify touch diagnostic test001 artifacts" in labels
    assert "SP01: flash touch diagnostic test001" in labels


def test_touch_diag_profile_binary() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "touch_diag_test.exe"
    cmd = [
        "gcc",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "touch_diag_test.c"),
        str(ROOT / "app" / "sp01_touch_diag_test.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)

    main_cmd = [
        "gcc",
        "-DHOST_SIM",
        "-DSP01_TOUCH_DIAG_TEST",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        "-c",
        str(ROOT / "main.c"),
        "-o",
        str(BUILD / "main_touch_diag_test.o"),
    ]
    subprocess.run(main_cmd, cwd=REPO, check=True)


def test_spec_product_profile_binary() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "spec_product_profile_test.exe"
    cmd = [
        "gcc",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "spec_product_profile_test.c"),
        str(ROOT / "app" / "sp01_spec_product.c"),
        str(ROOT / "app" / "sp01_touch_button.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_spec_product_profile004_binary() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "spec_product_profile_test004.exe"
    cmd = [
        "gcc",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-DSP01_SPEC_TEST004",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "spec_product_profile_test.c"),
        str(ROOT / "app" / "sp01_spec_product.c"),
        str(ROOT / "app" / "sp01_touch_button.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_spec_product_profile005_low_water_binary() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "spec_product_profile_test005.exe"
    cmd = [
        "gcc",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-DSP01_SPEC_TEST",
        "-DSP01_SPEC_TEST005",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "spec_product_profile_test.c"),
        str(ROOT / "app" / "sp01_spec_product.c"),
        str(ROOT / "app" / "sp01_touch_button.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_spec_product_profile006_low_water_recovery_binary() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "spec_product_profile_test006.exe"
    cmd = [
        "gcc",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-DSP01_SPEC_TEST",
        "-DSP01_SPEC_TEST006",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "spec_product_profile_test.c"),
        str(ROOT / "app" / "sp01_spec_product.c"),
        str(ROOT / "app" / "sp01_touch_button.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_spec_product_profile007_two_level_low_water_binary() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "spec_product_profile_test007.exe"
    cmd = [
        "gcc",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-DSP01_SPEC_TEST",
        "-DSP01_SPEC_TEST007",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "spec_product_profile_test.c"),
        str(ROOT / "app" / "sp01_spec_product.c"),
        str(ROOT / "app" / "sp01_touch_button.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_low_water_test_profile_is_explicit() -> None:
    build_info = read("app/sp01_build_info.h")
    full_source = read("app/sp01_full_test.c")
    main = read("main.c")
    makefile = read("target/py32f002b/Makefile")
    build_script = read_repo("tools/build-sp01-firmware.ps1")
    verify_script = read_repo("tools/verify-sp01-artifacts.ps1")
    tasks = json.loads(read_repo(".vscode/tasks.json"))

    assert "SP01_LOW_WATER_TEST" in build_info
    assert "SP01_BUILD_PROFILE_LOW_WATER_TEST" in main
    assert "LOW_WATER_TEST" in makefile
    assert "SP01_PUMP_DIAGNOSTICS_ENABLED=1" in makefile
    assert "立即停泵并锁定" in full_source
    assert "[switch]$LowWaterTest001" in build_script
    assert '"LOW_WATER_TEST=1"' in build_script
    assert "[switch]$LowWaterTest001" in verify_script
    assert '"build-low-water-test001"' in verify_script
    labels = {task["label"] for task in tasks["tasks"]}
    assert "SP01: low water test001 build" in labels
    assert "SP01: verify low water test001 artifacts" in labels
    assert "SP01: flash low water test001" in labels


def test_rgb_test_profile_is_channel_only() -> None:
    build_info = read("app/sp01_build_info.h")
    rgb_header = read("app/sp01_rgb_test.h")
    rgb_source = read("app/sp01_rgb_test.c")
    main = read("main.c")
    makefile = read("target/py32f002b/Makefile")
    build_script = read_repo("tools/build-sp01-firmware.ps1")
    verify_script = read_repo("tools/verify-sp01-artifacts.ps1")
    tasks = json.loads(read_repo(".vscode/tasks.json"))

    assert "SP01_RGB_TEST cannot be combined" in build_info
    assert "SP01_BUILD_PROFILE_RGB_TEST" in main
    assert "sp01_rgb_test_tick" in main
    assert "SP01_RGB_TEST" in makefile
    assert "sp01_rgb_test.c" in makefile
    assert "SP01_LED_TEST_PB3" in rgb_header or "SP01_LED_TEST_PB3" in rgb_source
    assert "SP01_LED_TEST_PA5" in rgb_source
    assert "SP01_LED_TEST_PA1" in rgb_source
    assert "outputs.pump_enable = APP_FALSE" in rgb_source
    assert "outputs.radar_power_enable = APP_FALSE" in rgb_source
    assert "outputs.buzzer_mode = SP01_BUZZER_OFF" in rgb_source
    assert "[switch]$RgbTest001" in build_script
    assert '"RGB_TEST=1"' in build_script
    assert "[switch]$RgbTest001" in verify_script
    assert '"build-rgb-test001"' in verify_script
    labels = {task["label"] for task in tasks["tasks"]}
    assert "SP01: rgb test001 build" in labels
    assert "SP01: verify rgb test001 artifacts" in labels
    assert "SP01: flash rgb test001" in labels

    assert "SP01_RGB_SINGLE_TEST cannot be combined" in build_info
    assert "SP01_RGB_ISOLATED_PA5_TEST cannot be combined" in build_info
    assert "SP01_RGB_ISOLATED_PB3_TEST cannot be combined" in build_info
    assert "SP01_RGB_COMBO_PA5_PA1_TEST cannot be combined" in build_info
    assert "SP01_RGB_COMBO_SCAN_TEST cannot be combined" in build_info
    assert "SP01_RGB_ALL_ON_TEST cannot be combined" in build_info
    assert "SP01_RGB_SINGLE_TEST" in makefile
    assert "RGB_CHANNEL" in makefile
    assert "RgbSingleChannel" in build_script
    assert "RgbSingleChannel" in verify_script
    assert "RgbSingleActiveHigh" in build_script
    assert "SP01_RGB_SINGLE_ACTIVE_HIGH" in makefile
    assert "SP01_RGB_ISOLATED_PA5_TEST" in makefile
    assert "SP01_RGB_ISOLATED_PB3_TEST" in makefile
    assert "SP01_RGB_COMBO_PA5_PA1_TEST" in makefile
    assert "SP01_RGB_COMBO_SCAN_TEST" in makefile
    assert "SP01_RGB_ALL_ON_TEST" in makefile
    assert "RgbAllOnTest001" in build_script
    assert '"RGB_ALL_ON_TEST=1"' in build_script
    assert "RgbAllOnTest001" in verify_script
    assert '"build-rgb-all-on-test001"' in verify_script
    for label in (
        "SP01: rgb single pa5 test001 build",
        "SP01: rgb single pa1 test001 build",
        "SP01: rgb single pb3 test001 build",
        "SP01: flash rgb single pa5 test001",
        "SP01: flash rgb single pa1 test001",
        "SP01: flash rgb single pb3 test001",
        "SP01: rgb single high pa5 test001 build",
        "SP01: rgb single high pb3 test001 build",
        "SP01: flash rgb single high pa5 test001",
        "SP01: flash rgb single high pb3 test001",
        "SP01: rgb isolated pa5 test001 build",
        "SP01: flash rgb isolated pa5 test001",
        "SP01: rgb isolated pb3 test001 build",
        "SP01: flash rgb isolated pb3 test001",
        "SP01: rgb combo pa5 pa1 test001 build",
        "SP01: flash rgb combo pa5 pa1 test001",
        "SP01: rgb combo scan test001 build",
        "SP01: flash rgb combo scan test001",
        "SP01: rgb all on test001 build",
        "SP01: verify rgb all on test001 artifacts",
        "SP01: flash rgb all on test001",
    ):
        assert label in labels


def test_full_test_profile_binary() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "full_test_profile_test.exe"
    cmd = [
        "gcc",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "full_test_profile_test.c"),
        str(ROOT / "app" / "sp01_full_test.c"),
        str(ROOT / "app" / "sp01_touch_button.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_rgb_test_profile_binary() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "rgb_test_profile_test.exe"
    cmd = [
        "gcc",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "rgb_test_profile_test.c"),
        str(ROOT / "app" / "sp01_rgb_test.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_rgb_single_test_profile_binary() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "rgb_single_test_profile_test.exe"
    cmd = [
        "gcc",
        "-DSP01_RGB_SINGLE_TEST",
        "-DSP01_RGB_CHANNEL_PA5",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "rgb_single_test_profile_test.c"),
        str(ROOT / "app" / "sp01_rgb_test.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_rgb_isolated_pa5_test_profile_binary() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "rgb_isolated_pa5_test_profile_test.exe"
    cmd = [
        "gcc",
        "-DSP01_RGB_ISOLATED_PA5_TEST",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "rgb_isolated_pa5_test_profile_test.c"),
        str(ROOT / "app" / "sp01_rgb_test.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_rgb_isolated_pb3_test_profile_binary() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "rgb_isolated_pb3_test_profile_test.exe"
    cmd = [
        "gcc",
        "-DSP01_RGB_ISOLATED_PB3_TEST",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "rgb_isolated_pb3_test_profile_test.c"),
        str(ROOT / "app" / "sp01_rgb_test.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_rgb_combo_pa5_pa1_test_profile_binary() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "rgb_combo_pa5_pa1_test_profile_test.exe"
    cmd = [
        "gcc",
        "-DSP01_RGB_COMBO_PA5_PA1_TEST",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "rgb_combo_pa5_pa1_test_profile_test.c"),
        str(ROOT / "app" / "sp01_rgb_test.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_rgb_combo_scan_test_profile_binary() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "rgb_combo_scan_test_profile_test.exe"
    cmd = [
        "gcc",
        "-DSP01_RGB_COMBO_SCAN_TEST",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "rgb_combo_scan_test_profile_test.c"),
        str(ROOT / "app" / "sp01_rgb_test.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_rgb_all_on_test_profile_binary() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "rgb_all_on_test_profile_test.exe"
    cmd = [
        "gcc",
        "-DSP01_RGB_ALL_ON_TEST",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "rgb_all_on_test_profile_test.c"),
        str(ROOT / "app" / "sp01_rgb_test.c"),
        str(ROOT / "app" / "sp01_indicator.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_rgb_white_balance_test_profile_binary() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "rgb_white_balance_test_profile_test.exe"
    cmd = [
        "gcc",
        "-DSP01_RGB_WHITE_BALANCE_TEST",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "rgb_white_balance_test_profile_test.c"),
        str(ROOT / "app" / "sp01_rgb_test.c"),
        str(ROOT / "app" / "sp01_indicator.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)

    build_info = read("app/sp01_build_info.h")
    main = read("main.c")
    vendor_port = read("platform/py32f002b/vendor_port.c")
    interrupt = read("platform/py32f002b/py32f002b_it.c")
    build_script = read_repo("tools/build-sp01-firmware.ps1")
    verify_script = read_repo("tools/verify-sp01-artifacts.ps1")

    assert "SP01_BUILD_PROFILE_RGB_WHITE_BALANCE_TEST" in build_info
    assert "SP01_RGB_WHITE_BALANCE_TEST cannot be combined" in build_info
    assert "SP01_BUILD_PROFILE_RGB_WHITE_BALANCE_TEST" in main
    assert "RGB_WHITE_BALANCE_TEST001" in build_info
    assert "board_vendor_rgb_pwm_tick_1ms" in interrupt
    assert "SP01_RGB_WHITE_BALANCE_PWM_PERIOD_MS" in vendor_port
    assert "RgbWhiteBalanceTest001" in build_script
    assert '"RGB_WHITE_BALANCE_TEST=1"' in build_script
    assert "RgbWhiteBalanceTest001" in verify_script
    assert '"build-rgb-white-balance-test001"' in verify_script


def test_board_profile_matches_schematic_names() -> None:
    profile = read("app/board_profile.h")
    adc_msp = read("platform/py32f002b/py32f002b_hal_msp.c")
    vendor_port = read("platform/py32f002b/vendor_port.c")
    for signal in (
        "BOARD_PIN_PUMP_ON",
        "BOARD_PIN_ADC_PUMP",
        "BOARD_PIN_ADC_VBAT",
        "BOARD_PIN_NTC_ADC",
        "BOARD_PIN_CHARGE_ACTIVE",
        "BOARD_PIN_CHARGE_DONE",
        "BOARD_PUMP_SHUNT_MILLIOHM",
    ):
        assert signal in profile
    assert "330U" in profile
    assert '"PA3/ADC_IN1"' in profile
    assert 'BOARD_PIN_LED_RED            "PB3"' in profile
    assert 'BOARD_PIN_LED_GREEN          "PA5"' in profile
    assert 'BOARD_PIN_LED_BLUE           "PA1"' in profile
    assert "case BOARD_VENDOR_PIN_LED_RED:\n        return GPIO_PIN_3;" in vendor_port
    assert "case BOARD_VENDOR_PIN_LED_GREEN:\n        return GPIO_PIN_5;" in vendor_port
    assert "case BOARD_VENDOR_PIN_LED_BLUE:\n        return GPIO_PIN_1;" in vendor_port
    assert "GPIO_PIN_3 | GPIO_PIN_6" in adc_msp
    assert "#define BOARD_KEY_ACTIVE_LEVEL            BOARD_GPIO_ACTIVE_LOW" in profile


def test_target_port_is_explicitly_guarded() -> None:
    board_port = read("platform/py32f002b/board_port.c")
    vendor_api = read("platform/py32f002b/board_port_vendor.h")
    template = read("platform/py32f002b/vendor_port_template.c")
    vendor_port = read("platform/py32f002b/vendor_port.c")
    hal_conf = read("platform/py32f002b/py32f002b_hal_conf.h")
    main = read("main.c")

    assert "SP01_PY32_VENDOR_PORT" in board_port
    assert "board_vendor_gpio_write" in vendor_api
    assert "board_vendor_rgb_isolated_apply" in vendor_api
    assert "board_vendor_adc_read_mv" in vendor_api
    assert "TODO" in template
    assert "return GPIOC" in vendor_port
    assert "ADC_CHANNEL_1" in vendor_port
    assert "ADC_CHANNEL_0" in vendor_port
    assert "ADC_CHANNEL_3" in vendor_port
    assert "GPIO_AF5_TIM14" in vendor_port
    assert "HAL_TIM_PWM_Init" in vendor_port
    assert "HAL_TIM_PWM_Start" in vendor_port
    assert "HAL_IWDG_Init" in vendor_port
    assert "HAL_IWDG_Refresh" in vendor_port
    assert "IWDG_PRESCALER_64" in vendor_port
    assert "HAL_IWDG_MODULE_ENABLED" in hal_conf
    assert "board_watchdog_refresh" in main
    assert "g_sp01_debug_reset_reason" in main
    assert "g_sp01_debug_watchdog_refresh_enabled" in main
    assert "g_sp01_debug_uptime_ms" in main
    assert "g_sp01_debug_radar_event" in main
    assert "g_sp01_debug_charge_status" in main
    assert "TIM14" in vendor_port


def test_input_modules_are_integrated() -> None:
    main = read("main.c")
    makefile = read("target/py32f002b/Makefile")
    radar_header = read("app/sp01_radar_input.h")
    charge_header = read("app/sp01_charge_monitor.h")

    assert "sp01_radar_input_update" in main
    assert "sp01_charge_monitor_update" in main
    assert "sp01_radar_input.c" in makefile
    assert "sp01_charge_monitor.c" in makefile
    assert "rising_edge" in radar_header
    assert "conflict" in charge_header


def test_uncalibrated_pump_diagnostics_default_to_observation_only() -> None:
    config = read("app/app_config.h")
    assert "#define SP01_PUMP_DIAGNOSTICS_ENABLED          0U" in config

    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "pump_monitor_uncalibrated_test.exe"
    cmd = [
        "gcc",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "pump_monitor_uncalibrated_test.c"),
        str(ROOT / "app" / "sp01_pump_monitor.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_pump_test_requires_deliberate_hold_and_runs_once() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "pump_test_profile_test.exe"
    cmd = [
        "gcc",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "pump_test_profile_test.c"),
        str(ROOT / "app" / "sp01_pump_test.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_pump_adc_test_profile_binary() -> None:
    build_script = read_repo("tools/build-sp01-firmware.ps1")
    verify_script = read_repo("tools/verify-sp01-artifacts.ps1")
    makefile = read("target/py32f002b/Makefile")
    tasks = json.loads(read_repo(".vscode/tasks.json"))
    assert "[switch]$PumpAdcTest001" in build_script
    assert '"PUMP_ADC_TEST=1"' in build_script
    assert "[switch]$PumpAdcTest001" in verify_script
    assert '"build-pump-adc-test001"' in verify_script
    assert "SP01_PUMP_ADC_CHANNEL" in read("app/app_config.h")
    assert "PUMP_ADC_CHANNEL" in build_script
    assert "g_sp01_debug_adc_scan_raw" in read("main.c")
    assert "g_sp01_debug_adc_selected_average_raw" in read("main.c")
    assert "g_sp01_debug_adc_selected_average_mv" in read("main.c")
    assert "g_sp01_debug_adc_stable" in read("main.c")
    assert "PUMP_ADC_CHANNEL=0" in build_script
    assert "PUMP_ADC_CHANNEL=1" in build_script
    assert "PUMP_ADC_CHANNEL=3" in build_script
    assert "SP01_PUMP_ADC_TEST" in makefile
    labels = {task["label"] for task in tasks["tasks"]}
    for channel in ("PA3", "PB1", "PA6"):
        assert f"SP01: pump ADC {channel} test001 build" in labels
        assert f"SP01: flash pump ADC {channel} test001" in labels

    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "pump_adc_test_profile_test.exe"
    cmd = [
        "gcc",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "pump_adc_test_profile_test.c"),
        str(ROOT / "app" / "sp01_pump_adc_test.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_pump_adc_hold_test_profile_binary() -> None:
    build_script = read_repo("tools/build-sp01-firmware.ps1")
    verify_script = read_repo("tools/verify-sp01-artifacts.ps1")
    makefile = read("target/py32f002b/Makefile")
    tasks = json.loads(read_repo(".vscode/tasks.json"))
    assert "[switch]$PumpAdcHoldTest002" in build_script
    assert '"PUMP_ADC_HOLD_TEST=1"' in build_script
    assert '"PUMP_ADC_CHANNEL=1"' in build_script
    assert "[switch]$PumpAdcHoldTest002" in verify_script
    assert '"build-pump-adc-hold-test002"' in verify_script
    assert "SP01_PUMP_ADC_HOLD_TEST" in makefile
    assert "SP01_PUMP_ADC_HOLD_TEST" in read("main.c")
    labels = {task["label"] for task in tasks["tasks"]}
    assert "SP01: pump ADC hold test002 build" in labels
    assert "SP01: flash pump ADC hold test002" in labels

    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "pump_adc_hold_test_profile_test.exe"
    cmd = [
        "gcc",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "pump_adc_hold_test_profile_test.c"),
        str(ROOT / "app" / "sp01_pump_adc_hold_test.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_battery_pump_profile_stays_on_for_two_minutes() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "battery_pump_test_profile_test.exe"
    cmd = [
        "gcc",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "battery_pump_test_profile_test.c"),
        str(ROOT / "app" / "sp01_battery_pump_test.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_vscode_tasks_are_runnable() -> None:
    tasks = json.loads(read_repo(".vscode/tasks.json"))
    by_label = {task["label"]: task for task in tasks["tasks"]}

    for label in ("SP01: firmware checks", "SP01: module tests"):
        task = by_label[label]
        args = task["args"]
        assert task["command"] == "shnote"
        assert "python" in args
        normalized_args = [arg.replace("/", "\\") for arg in args]
        assert "firmware\\tests\\test_firmware_sources.py" in normalized_args
        assert "-f" not in args

    for label in (
        "SP01: target build",
        "SP01: release build",
        "SP01: bench no-pump build",
        "SP01: radar test build",
        "SP01: pump test build",
        "SP01: battery test build",
        "SP01: battery pump cycle test build",
        "SP01: full test001 build",
        "SP01: rgb test001 build",
    ):
        task = by_label[label]
        assert task["command"] == "shnote"
        normalized_args = [arg.replace("/", "\\") for arg in task["args"]]
        assert "tools\\build-sp01-firmware.ps1" in normalized_args

    radar_flash = by_label["SP01: flash radar test"]
    assert "build-radar-test" in radar_flash["args"][-1]
    assert radar_flash["dependsOn"] == "SP01: radar test build"

    pump_flash = by_label["SP01: flash pump test"]
    assert "build-pump-test" in pump_flash["args"][-1]
    assert pump_flash["dependsOn"] == "SP01: pump test build"

    battery_flash = by_label["SP01: flash battery test"]
    assert "build-battery-test" in battery_flash["args"][-1]
    assert battery_flash["dependsOn"] == "SP01: battery test build"

    battery_pump_flash = by_label["SP01: flash battery pump cycle test"]
    assert "build-battery-pump-test" in battery_pump_flash["args"][-1]
    assert battery_pump_flash["dependsOn"] == "SP01: battery pump cycle test build"

    full_test_flash = by_label["SP01: flash full test001"]
    assert "build-full-test001" in full_test_flash["args"][-1]
    assert full_test_flash["dependsOn"] == "SP01: full test001 build"

    rgb_test_flash = by_label["SP01: flash rgb test001"]
    assert "build-rgb-test001" in rgb_test_flash["args"][-1]
    assert rgb_test_flash["dependsOn"] == "SP01: rgb test001 build"

    for channel in ("pa5", "pa1", "pb3"):
        flash = by_label[f"SP01: flash rgb single {channel} test001"]
        assert f"build-rgb-single-{channel}-test001" in flash["args"][-1]
        assert flash["dependsOn"] == f"SP01: rgb single {channel} test001 build"

    for channel in ("pa5", "pb3"):
        flash = by_label[f"SP01: flash rgb single high {channel} test001"]
        assert f"build-rgb-single-high-{channel}-test001" in flash["args"][-1]
        assert flash["dependsOn"] == f"SP01: rgb single high {channel} test001 build"

    launch = json.loads(read_repo(".vscode/launch.json"))
    bench_config = next(
        item for item in launch["configurations"]
        if item["name"] == "PY32F002B: Type-C 台架（强制关泵）"
    )
    assert "build-bench-no-pump" in bench_config["executable"]
    assert bench_config["preLaunchTask"] == "SP01: bench no-pump debug preflight"


def test_no_blocking_hardware_waits() -> None:
    for path in project_sources("*.c"):
        text = path.read_text(encoding="utf-8")
        assert "while (!" not in text
        assert "while(!" not in text


def test_host_unit_binary() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "sp01_host_tests.exe"
    cmd = [
        "gcc",
        "-DHOST_SIM",
        "-DSP01_PUMP_DIAGNOSTICS_ENABLED=1",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "host_sp01_test.c"),
        str(ROOT / "app" / "sp01_app.c"),
        str(ROOT / "app" / "sp01_buzzer_pattern.c"),
        str(ROOT / "app" / "sp01_power_monitor.c"),
        str(ROOT / "app" / "sp01_signal_filter.c"),
        str(ROOT / "app" / "sp01_indicator.c"),
        str(ROOT / "app" / "sp01_touch_button.c"),
        str(ROOT / "app" / "sp01_radar_input.c"),
        str(ROOT / "app" / "sp01_charge_monitor.c"),
        str(ROOT / "app" / "sp01_pump_monitor.c"),
        str(ROOT / "platform" / "py32f002b" / "board_port.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_main_and_board_port_compile() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    cmd = [
        "gcc",
        "-DHOST_SIM",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        "-c",
        str(ROOT / "main.c"),
        "-o",
        str(BUILD / "main.o"),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)

    radar_test_cmd = [
        "gcc",
        "-DHOST_SIM",
        "-DSP01_RADAR_TEST",
        "-DSP01_BENCH_NO_PUMP",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        "-c",
        str(ROOT / "main.c"),
        "-o",
        str(BUILD / "main_radar_test.o"),
    ]
    subprocess.run(radar_test_cmd, cwd=REPO, check=True)

    pump_test_cmd = [
        "gcc",
        "-DHOST_SIM",
        "-DSP01_PUMP_TEST",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        "-c",
        str(ROOT / "main.c"),
        "-o",
        str(BUILD / "main_pump_test.o"),
    ]
    subprocess.run(pump_test_cmd, cwd=REPO, check=True)

    battery_pump_test_cmd = [
        "gcc",
        "-DHOST_SIM",
        "-DSP01_BATTERY_PUMP_TEST",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        "-c",
        str(ROOT / "main.c"),
        "-o",
        str(BUILD / "main_battery_pump_test.o"),
    ]
    subprocess.run(battery_pump_test_cmd, cwd=REPO, check=True)

    full_test_cmd = [
        "gcc",
        "-DHOST_SIM",
        "-DSP01_FULL_TEST",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        "-c",
        str(ROOT / "main.c"),
        "-o",
        str(BUILD / "main_full_test.o"),
    ]
    subprocess.run(full_test_cmd, cwd=REPO, check=True)

    low_water_test_cmd = [
        "gcc",
        "-DHOST_SIM",
        "-DSP01_LOW_WATER_TEST",
        "-DSP01_FULL_TEST",
        "-DSP01_PUMP_DIAGNOSTICS_ENABLED=1",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        "-c",
        str(ROOT / "main.c"),
        "-o",
        str(BUILD / "main_low_water_test.o"),
    ]
    subprocess.run(low_water_test_cmd, cwd=REPO, check=True)

    rgb_test_cmd = [
        "gcc",
        "-DHOST_SIM",
        "-DSP01_RGB_TEST",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        "-c",
        str(ROOT / "main.c"),
        "-o",
        str(BUILD / "main_rgb_test.o"),
    ]
    subprocess.run(rgb_test_cmd, cwd=REPO, check=True)

    rgb_single_test_cmd = [
        "gcc",
        "-DHOST_SIM",
        "-DSP01_RGB_SINGLE_TEST",
        "-DSP01_RGB_CHANNEL_PA5",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        "-c",
        str(ROOT / "main.c"),
        "-o",
        str(BUILD / "main_rgb_single_test.o"),
    ]
    subprocess.run(rgb_single_test_cmd, cwd=REPO, check=True)

    rgb_isolated_test_cmd = [
        "gcc",
        "-DHOST_SIM",
        "-DSP01_RGB_ISOLATED_PA5_TEST",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        "-c",
        str(ROOT / "main.c"),
        "-o",
        str(BUILD / "main_rgb_isolated_pa5_test.o"),
    ]
    subprocess.run(rgb_isolated_test_cmd, cwd=REPO, check=True)

    rgb_isolated_pb3_test_cmd = [
        "gcc",
        "-DHOST_SIM",
        "-DSP01_RGB_ISOLATED_PB3_TEST",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        "-c",
        str(ROOT / "main.c"),
        "-o",
        str(BUILD / "main_rgb_isolated_pb3_test.o"),
    ]
    subprocess.run(rgb_isolated_pb3_test_cmd, cwd=REPO, check=True)

    cmd[1:1] = ["-DSP01_BENCH_NO_PUMP"]
    cmd[cmd.index("-o") + 1] = str(BUILD / "main_bench_no_pump.o")
    subprocess.run(cmd, cwd=REPO, check=True)

    cmd = [
        "gcc",
        "-DHOST_SIM",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        "-c",
        str(ROOT / "platform" / "py32f002b" / "board_port.c"),
        "-o",
        str(BUILD / "board_port.o"),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)


def test_bench_no_pump_interlock() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    exe = BUILD / "bench_no_pump_interlock_test.exe"
    cmd = [
        "gcc",
        "-DHOST_SIM",
        "-DSP01_BENCH_NO_PUMP",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        str(ROOT / "tests" / "bench_no_pump_interlock_test.c"),
        str(ROOT / "platform" / "py32f002b" / "board_port.c"),
        str(ROOT / "app" / "sp01_indicator.c"),
        "-o",
        str(exe),
    ]
    subprocess.run(cmd, cwd=REPO, check=True)
    subprocess.run([str(exe)], cwd=REPO, check=True)


def test_bench_and_release_builds_are_mutually_exclusive() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    cmd = [
        "gcc",
        "-DHOST_SIM",
        "-DSP01_BENCH_NO_PUMP",
        "-DSP01_RELEASE_BUILD",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        "-c",
        str(ROOT / "main.c"),
        "-o",
        str(BUILD / "invalid_combined_profile.o"),
    ]
    result = subprocess.run(cmd, cwd=REPO, capture_output=True, text=True)
    assert result.returncode != 0
    assert "cannot be combined" in result.stderr


def test_full_test_and_release_builds_are_mutually_exclusive() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    cmd = [
        "gcc",
        "-DHOST_SIM",
        "-DSP01_FULL_TEST",
        "-DSP01_RELEASE_BUILD",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        "-c",
        str(ROOT / "main.c"),
        "-o",
        str(BUILD / "invalid_full_test_release.o"),
    ]
    result = subprocess.run(cmd, cwd=REPO, capture_output=True, text=True)
    assert result.returncode != 0
    assert "cannot be combined" in result.stderr


def test_pump_test_and_release_builds_are_mutually_exclusive() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    cmd = [
        "gcc",
        "-DHOST_SIM",
        "-DSP01_PUMP_TEST",
        "-DSP01_RELEASE_BUILD",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(ROOT),
        "-I",
        str(ROOT / "app"),
        "-c",
        str(ROOT / "main.c"),
        "-o",
        str(BUILD / "invalid_pump_test_release.o"),
    ]
    result = subprocess.run(cmd, cwd=REPO, capture_output=True, text=True)
    assert result.returncode != 0
    assert "cannot be combined" in result.stderr


if __name__ == "__main__":
    tests = [
        test_no_old_dialogue_project_names,
        test_sp01_state_machine_contract_is_present,
        test_build_info_is_present,
        test_radar_test_profile_beeps_on_debounced_rising_edge,
        test_pump_test_profile_is_continuous_and_separate,
        test_battery_test_profile_is_safe_and_observable,
        test_battery_pump_profile_is_always_on,
        test_full_test_profile_is_type_c_only,
        test_spec_product_profile_is_supply_aware,
        test_spec_product_profile_binary,
        test_spec_product_profile004_binary,
        test_spec_product_profile005_low_water_binary,
        test_spec_product_profile006_low_water_recovery_binary,
        test_spec_product_profile007_two_level_low_water_binary,
        test_touch_diag_profile_binary,
        test_full_test_profile_binary,
        test_low_water_test_profile_is_explicit,
        test_rgb_test_profile_is_channel_only,
        test_rgb_test_profile_binary,
        test_rgb_single_test_profile_binary,
        test_rgb_isolated_pa5_test_profile_binary,
        test_rgb_isolated_pb3_test_profile_binary,
        test_rgb_combo_pa5_pa1_test_profile_binary,
        test_rgb_combo_scan_test_profile_binary,
        test_rgb_all_on_test_profile_binary,
        test_rgb_white_balance_test_profile_binary,
        test_battery_pump_profile_stays_on_for_two_minutes,
        test_board_profile_matches_schematic_names,
        test_input_modules_are_integrated,
        test_uncalibrated_pump_diagnostics_default_to_observation_only,
        test_pump_test_requires_deliberate_hold_and_runs_once,
        test_pump_adc_test_profile_binary,
        test_pump_adc_hold_test_profile_binary,
        test_vscode_tasks_are_runnable,
        test_no_blocking_hardware_waits,
        test_host_unit_binary,
        test_main_and_board_port_compile,
        test_bench_no_pump_interlock,
        test_bench_and_release_builds_are_mutually_exclusive,
        test_pump_test_and_release_builds_are_mutually_exclusive,
        test_full_test_and_release_builds_are_mutually_exclusive,
    ]
    for test in tests:
        test()
        print(f"PASS {test.__name__}")
