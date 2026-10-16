# PY32F002B 目标构建

从仓库根目录运行 `tools/build-sp01-firmware.ps1 -SpecTest007Retry30s`；先安装 Arm GNU Toolchain 和 GNU Make，必要时传入 `-ToolchainBin` 与 `-MakePath`。详细步骤见 [构建文档](../../../docs/build-and-flash.md)。

Makefile 接入普冉 SDK 的启动文件、系统文件、链接脚本和 HAL，目标配置为 Cortex-M0+、24 KB Flash / 3 KB RAM。当前目标构建使用 Windows 命令语法，其他系统尚未验证。

本目录下的 `build*` 为本地生成文件，默认不提交；不能混用不同 profile 的 HEX、BIN 和 ELF。
