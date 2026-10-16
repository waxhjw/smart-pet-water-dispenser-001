# SP01 固件

当前学习基线是 `SPEC_TEST007_LOW_WATER_WARNING`。从 [项目 README](../README.md)、[构建与烧录](../docs/build-and-flash.md)和 [007 功能](../docs/spec-test007.md)开始阅读。

`app/` 管产品逻辑，`platform/py32f002b/` 管板级外设，`target/py32f002b/` 管目标构建，`tests/` 管主机测试，`vendor/` 为保留原许可的 SDK。

保留历史专项模式是为了追溯与回归。PumpTest、BatteryPumpTest 持续开泵，不能作为日常整机固件。发布整理未改动应用控制逻辑，也未执行硬件烧录。
