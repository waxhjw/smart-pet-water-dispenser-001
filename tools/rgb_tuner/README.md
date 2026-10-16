# SP01 RGB 调色工具

用于历史 RGB 专项调试，通过 pyOCD/SWD 修改专用调色固件的占空比变量。与 ADC 只读工具不同，本工具会写入目标 RAM。

```powershell
python -m pip install -r tools/rgb_tuner/requirements.txt
python tools/rgb_tuner/rgb_tuner.py
```

仅配套 `-RgbTunerTest001` 固件和同次构建的 `build-rgb-tuner-test001/sp01_py32f002b.elf` 使用；不能直接用于 007 整机固件。该 profile 的板级输出强制关泵。上位机根据专用符号与 profile 检查目标是否匹配。

接好 SWDIO、SWCLK、GND 和参考电压，连接后逐路从低占空比开始调整，观察灯色，记录哪一引脚实际对应红、绿、蓝。通过标准是能独立控制对应通道、松开/改变滑块后输出跟随且泵保持关闭。

界面保留历史 PB3=R、PA5=G、PA1=B 命名；当前样机记录中红蓝物理映射可能相反。核对实物后再记录白平衡比例，不能只看 R/B 标签。若颜色不符，优先核对 LED 连接、板卡版本、实际通道和 HEX/ELF 配对。本次公开整理只检查 Python 语法，未重新运行 GUI 或硬件调色测试。
