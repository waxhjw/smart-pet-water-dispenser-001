# SP01 宠物智能饮水机 · 非商业学习版

基于普冉 PY32F002B 的宠物饮水机项目，公开固件源码、板级适配、测试、调试工具和 SP01 硬件设计，供非商业学习、实验和修改参考。

**允许非商业使用和修改；商业用途须另行取得权利人的书面授权。** 固件与工具采用 [PolyForm Noncommercial 1.0.0](LICENSE)，项目原创文档及硬件设计采用 [CC BY-NC 4.0](LICENSE-DOCS-HARDWARE)。第三方 SDK 保留原许可证。由于存在非商业限制，本项目属于源码公开项目，不属于 OSI 定义的开源软件。

这是开发测试基线，**不是已经完成整机验收的量产固件**。硬件文档与源码来自不同阶段，复刻前必须核对实际板卡；详见 [已知限制](docs/known-limitations.md)。

## 当前基线

当前推荐研究入口是 `SPEC_TEST007_LOW_WATER_WARNING`，构建选项为 `-SpecTest007Retry30s`。应用层、板级代码及测试从原开发基线 `bb66b63915236ea06edf1679d3ff34366d78c77a` 提取；公开版调整了构建工具的查找方式，未修改固件控制逻辑。文件校验记录见 [来源清单](docs/source-manifest.json)。原开发仓库历史没有随公开版发布。

| 项目 | 当前实现 |
| --- | --- |
| 主控 | PY32F002BF15U6TR，Cortex-M0+，24 KB Flash / 3 KB RAM 配置 |
| Type-C 模式 | 识别供电稳定后连续出水 |
| 电池模式 | 雷达触发出水约 15 秒，随后锁定约 3 分钟 |
| 供电切换 | 拔掉 Type-C 后经稳定确认切回电池雷达待机 |
| 触摸按键 | 长按约 3 秒开关机；短按切换夜间模式 |
| 水位预警 | PA3 均值连续达到 ≤585 mV，泵继续运行并单闪提示 |
| 缺水保护 | PA3 均值 ≤420 mV 连续 3 个窗口后停泵 |
| 缺水复测 | 每 30 秒受控试泵，每次最长 5 秒；>625 mV 连续两个窗口恢复 |
| 电池电压 | 低电量提示及临界低压关机，仍需板上回归 |

这些是代码行为，不代表全部通过实物验证。预警、保护阈值依赖线圈、负载和板卡，不能当成通用水位标准。电池模式还保留周期冲毛流程，见 [007 功能说明](docs/spec-test007.md)。

## 目录

```text
firmware/app/                    产品状态机与模块
firmware/platform/py32f002b/     引脚、采样、定时器和输出适配
firmware/target/py32f002b/       Makefile 构建入口
firmware/tests/                 源码检查与主机侧行为测试
firmware/vendor/                普冉 SDK 必需源码及原始许可证
hardware/                      原理图、PCB 工程、元器件清单及版本说明
tools/adc_monitor/             通过 SWD 读取采样快照的上位机
tools/                         构建和产物检查脚本
docs/                          构建、测试、限制与发布记录
.vscode/                       开发任务模板
```

## 构建和使用

目标固件构建目前以 **Windows + PowerShell + GNU Make** 为验证环境。需要 Python 3.9+、用于主机测试的 GCC，以及 Arm GNU Toolchain；原开发使用 Arm GNU Toolchain 15.2.Rel1。第三方 SDK 的构建所需源码已随仓库提供，无需下载公司内部资料。

在仓库根目录执行主机回归：

```powershell
python firmware/tests/test_firmware_sources.py
```

构建 007，Arm 工具链目录应避免空格；若工具链已在 PATH 中，可省略 `-ToolchainBin`：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build-sp01-firmware.ps1 -SpecTest007Retry30s -ToolchainBin C:/arm-gnu/bin
powershell -NoProfile -ExecutionPolicy Bypass -File tools/verify-sp01-artifacts.ps1 -SpecTest007Retry30s
```

输出文件为 `firmware/target/py32f002b/build-spec-test007-retry30s/sp01_py32f002b.hex`；同目录 `.elf` 用于调试和上位机。详细环境、首次关泵测试及烧录步骤见 [构建与烧录](docs/build-and-flash.md)，验收步骤见 [007 测试方法](docs/test-plan.md)。

## 阅读顺序

1. [板级连接与硬件版本](hardware/README.md)
2. [007 功能与参数](docs/spec-test007.md)
3. [构建与烧录](docs/build-and-flash.md)
4. [分段测试方法](docs/test-plan.md)
5. [已知限制](docs/known-limitations.md)
6. [非商业授权说明](docs/licensing.md)与[第三方声明](THIRD_PARTY_NOTICES.md)

## 使用边界

007 会在确认 Type-C 供电后自动进入连续出水；旧 `PumpTest`、`BatteryPumpTest` 更是持续开泵的历史诊断模式。初次接板请使用 `-BenchNoPump` 先核对安全输出，备好停泵和断电方式，再在有水、供电和接线已确认的条件下测试 007。不要把历史诊断固件当作整机固件。

供应商规格书、客户文件、采购资料、公司内网配置、账号凭据、工具链安装包及原开发历史不在本公开仓库中。公开授权不替代已有客户合同或单独授予的权利。
