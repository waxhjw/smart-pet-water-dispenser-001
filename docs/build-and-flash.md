# 构建、烧录和回滚

## 环境

目前已验证的目标构建环境为 Windows、PowerShell、GNU Make 和 Arm GNU Toolchain 15.2.Rel1。主机测试还需要 Python 3.9+ 与能在 PATH 中找到的 GCC。官方 SDK 的必要源码已经随仓库提供；Linux/macOS 目标构建尚未适配，因为原 Makefile 的目录命令采用 Windows cmd 语法。

安装 GNU Make 后，让 `mingw32-make` 或 `make` 可从终端找到；也可以向脚本传入 `-MakePath C:/mingw/bin/mingw32-make.exe`。Arm 工具链放在无空格目录，或通过 `-ToolchainBin` 指定其 `bin` 目录。脚本使用临时盘符映射避免工程中文路径影响链接，结束时解除映射。

`.vscode` 保留原任务模板，部分任务需要外部 shnote、pyOCD、芯片包和自行安装的工具链。新环境优先按本文命令操作；这些模板并非全部在新环境验证可运行。

## 先做主机测试

```powershell
python firmware/tests/test_firmware_sources.py
```

通过标准：进程返回 0，列出的测试全部 PASS。这里验证源码检查、模块行为和模拟状态机，不验证实际水位、电池、无线供电或 PCB。

## 初次接板先构建强制关泵版

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build-sp01-firmware.ps1 -BenchNoPump -ToolchainBin C:/arm-gnu/bin
powershell -NoProfile -ExecutionPolicy Bypass -File tools/verify-sp01-artifacts.ps1 -BenchNoPump
```

应烧录 `firmware/target/py32f002b/build-bench-no-pump/sp01_py32f002b.hex`。复位后先确认 PB2 为关泵电平，电源和采样正常；出现误动作先断开电源并检查实际烧录版本、引脚和驱动极性。

## 构建 007

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build-sp01-firmware.ps1 -SpecTest007Retry30s -ToolchainBin C:/arm-gnu/bin
powershell -NoProfile -ExecutionPolicy Bypass -File tools/verify-sp01-artifacts.ps1 -SpecTest007Retry30s
```

输出目录：`firmware/target/py32f002b/build-spec-test007-retry30s/`。

| 文件 | 用途 |
| --- | --- |
| `sp01_py32f002b.hex` | 地址信息完整的烧录文件 |
| `sp01_py32f002b.elf` | 符号、地址和调试信息；上位机应选同次构建的 ELF |
| `sp01_py32f002b.bin` | 裸二进制，仅在工具和地址设置已经明确时使用 |

确认产物检查通过并保存 SHA-256；不同工具链、源码或参数可能产生不同校验值。公开版自身构建记录见 [发布验证记录](publication-validation.md)。

## 烧录准备、操作和判定

1. **准备：** 先停止设备并断开负载电源，连接 SWDIO、SWCLK、GND 和正确的目标参考电压；确认烧录器端口和板卡丝印，不按线色猜接法。Type-C 和烧录器电源不得未经核对直接并联。
2. **选文件：** 在支持该 PY32F002B 型号的烧录工具中选择当前构建目录的 `sp01_py32f002b.hex`，芯片选择必须与实物一致。不要选择早期 `PumpTest` 或其他 build 目录。
3. **写入：** 执行擦除、编程和校验，保留工具的成功/失败日志；目标地址由 HEX 携带。这里没有把烧录工具版本、选项字节或接线自动套用到未知板卡。
4. **运行：** 首次关泵检查通过后，按 [测试方法](test-plan.md)准备有水负载再运行 007。007 在 Type-C 供电稳定后会自行开泵，必须在复位前准备好水和停机路径。
5. **通过：** 写入校验成功、构建 profile 对应、每项硬件行为都满足测试表；编程成功本身不是整机通过。

本次公开整理没有执行任何硬件烧录。可用 DAPLink/PY-LINK 和相关软件随具体样机确认，不能把本文当作已验证所有烧录器的承诺。

## 回滚

修改前保留可验证基线的 Git 提交、HEX、SHA-256、板卡版本和测试记录。需要回退时，在独立目录检出已验证的 tag/提交，重建或使用其已保存且校验一致的 HEX，再按同样的编程校验步骤烧录。公开学习版标签仅代表源码快照，不代表硬件量产认证。

## 缩写

- SDK（Software Development Kit）：芯片厂商提供的基础源码包。
- GCC（GNU Compiler Collection）：编译器；主机 GCC 跑模拟测试，Arm GCC 生成单片机代码。
- SWD（Serial Wire Debug）：单片机的两线调试接口，烧录器通过它读写程序及调试。
- ELF（Executable and Linkable Format）：带程序和符号地址的文件，上位机靠它找 RAM 中的数据。
- SHA-256：文件指纹；文件内容变化，指纹通常也会变化，用来核对是否选中了同一个产物。
