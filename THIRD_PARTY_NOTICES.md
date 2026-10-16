# 第三方组件

根目录的非商业许可证仅适用于项目有权授权的原创部分，不改变第三方组件原有授权。可以单独依原许可证使用第三方组件；这不等于获得 SP01 原创代码、文档或硬件设计的商业授权。

## 普冉 PY32F002B SDK

- 目录：`firmware/vendor/py32f002b_firmware/`。
- 导入版本：PY32F002B Firmware V1.2.1。
- 上游：[OpenPuya/PY32F002B_Firmware](https://github.com/OpenPuya/PY32F002B_Firmware)。
- 本地原始 ZIP SHA-256：`864f44e7bea0cc9f35df84c0921646195ad3eee01b4b6d9da8cece63a571af69`；ZIP 不随本仓库分发。
- SDK 根目录的 [BSD 3-Clause 许可证](firmware/vendor/py32f002b_firmware/LICENSE)和各源文件声明均已保留。部分 HAL 文件另含 STMicroelectronics 版权及 BSD 声明；CMSIS 文件含 Arm 原始版权及许可声明，应一并遵守。
- 公开快照仅包含 Drivers 和 GCC/EIDE 启动、链接所需文件，未包含上游 Git 历史、示例工程、工具包或安装程序。

## 外部运行及开发依赖

GCC、Arm GNU Toolchain、GNU Make、Python、pyOCD、pyelftools、PySide6、DAPLink 和 shnote 为外部依赖，未将其程序或安装包纳入本项目许可。安装和使用应遵守各自许可证。ADC 工具的依赖在 `tools/adc_monitor/requirements.txt` 中声明。

VS Code 历史任务模板使用 shnote 显示命令用途；命令行构建本身不依赖 shnote。模板中的本地调试器、工具链和芯片包路径需要按自己的环境调整。
