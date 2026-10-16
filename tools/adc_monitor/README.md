# SP01 ADC 监视工具

ADC（Analog-to-Digital Converter，模数转换器）把采样脚电压变为数字。工具通过 DAPLink 的 SWD 读取固件 RAM 中的快照，不通过 Type-C 传输数据，也不写 GPIO 控制水泵。

安装和启动：

```powershell
python -m pip install -r tools/adc_monitor/requirements.txt
python tools/adc_monitor/adc_monitor.py
```

连接 SWDIO、SWCLK、GND 和适当的目标参考电压；避免烧录器与主板电源未经核对并联。选择与实际烧录 HEX **同次构建、同目录的 ELF**，点击连接并观察 PA3/ADC_IN1。工具默认 ELF 是历史 `build-pump-adc-hold-test002`；监视 007 时必须主动选择 `build-spec-test007-retry30s/sp01_py32f002b.elf`。

预期：ADC 最新值、约 1 秒均值、原始码及序号更新。通过标准是同次 ELF 地址正确且能持续读取，不能把界面数值直接当成已经通过缺水保护。若数值不可能或不更新，先核对 HEX/ELF、采样通道、SWD 连线及是否读到当前固件。

界面提供的电流估算采用后期资料中的 100 mΩ 采样电阻与 20 倍放大参数，尚未按每块实物标定；公开的 2026-09-23 硬件资料不保证具备同样采样电路。只作为观察辅助，不把它当作通用水位或准确电流。
