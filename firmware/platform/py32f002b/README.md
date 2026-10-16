# PY32F002B 板级适配

`board_port.c` 连接产品输入输出，`vendor_port.c` 连接普冉 HAL，`py32f002b_it.c` 提供中断入口。板级引脚和有效电平见 [硬件说明](../../../hardware/README.md)。

007 使用 PB2 控泵、PA3/ADC_IN1 采样负载、PB1/ADC_IN0 采样电池，并使用定时器控制 RGB 和蜂鸣器。历史 `vendor_port_template.c` 是接口模板，不与实际实现同时加入构建。

实物红蓝通道与部分历史命名有不一致记录，见 [已知限制](../../../docs/known-limitations.md)。复用前逐路核对，不仅看变量名。
