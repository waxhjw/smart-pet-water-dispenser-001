from __future__ import annotations

import sys
from pathlib import Path
from typing import Optional

try:
    from PySide6.QtCore import QTimer, Qt
    from PySide6.QtWidgets import (
        QApplication,
        QGridLayout,
        QGroupBox,
        QHBoxLayout,
        QLabel,
        QMainWindow,
        QMessageBox,
        QPushButton,
        QSlider,
        QSpinBox,
        QVBoxLayout,
        QWidget,
    )
except ImportError as exc:
    raise SystemExit("缺少 PySide6，请运行 tools/rgb_tuner/install.ps1") from exc

try:
    from elftools.elf.elffile import ELFFile
    from pyocd.core.helpers import ConnectHelper
except ImportError as exc:
    raise SystemExit("缺少 pyOCD 或 pyelftools，请运行 tools/rgb_tuner/install.ps1") from exc


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_ELF = ROOT / "firmware/target/py32f002b/build-rgb-tuner-test001/sp01_py32f002b.elf"
EXPECTED_PROFILE = 27
PROFILE_SYMBOL = "g_sp01_rgb_tuner_profile_id"
CHANNEL_SYMBOLS = (
    "g_sp01_rgb_tuner_red_duty_percent",
    "g_sp01_rgb_tuner_green_duty_percent",
    "g_sp01_rgb_tuner_blue_duty_percent",
)
CHANNEL_NAMES = ("红色 R / PB3", "绿色 G / PA5", "蓝色 B / PA1")
CHANNEL_COLORS = ("#e53935", "#43a047", "#1e88e5")


class ElfSymbols:
    def __init__(self, path: Path):
        self.addresses: dict[str, int] = {}
        with path.open("rb") as stream:
            elf = ELFFile(stream)
            for section_name in (".symtab", ".dynsym"):
                section = elf.get_section_by_name(section_name)
                if section is None:
                    continue
                for symbol in section.iter_symbols():
                    if symbol.name and int(symbol.entry["st_value"]):
                        self.addresses[symbol.name] = int(symbol.entry["st_value"])

    def require(self, name: str) -> int:
        try:
            return self.addresses[name]
        except KeyError as exc:
            raise RuntimeError(f"ELF 文件缺少调色变量：{name}") from exc


class SwdRgbWriter:
    def __init__(self, elf_path: Path):
        symbols = ElfSymbols(elf_path)
        self.profile_address = symbols.require(PROFILE_SYMBOL)
        self.channel_addresses = [symbols.require(name) for name in CHANNEL_SYMBOLS]
        self.session = None
        self.target = None

    def connect(self) -> None:
        self.session = ConnectHelper.session_with_chosen_probe(
            return_first=True,
            auto_open=False,
            options={"frequency": 100_000, "target_override": "cortex_m"},
        )
        if self.session is None:
            raise RuntimeError("没有找到 DAPLink/CMSIS-DAP 探针，请检查 SWD 连接。")
        self.session.open()
        self.target = self.session.board.target
        profile = int(self.target.read32(self.profile_address))
        if profile != EXPECTED_PROFILE:
            self.close()
            raise RuntimeError(
                f"板上固件标识是 {profile}，不是 RGB_TUNER_TEST001（27）。\n"
                "请先烧录本目录对应的调色测试 HEX，再连接上位机。"
            )

    def close(self) -> None:
        if self.session is not None:
            self.session.close()
        self.session = None
        self.target = None

    def write_values(self, values: tuple[int, int, int]) -> None:
        if self.target is None:
            raise RuntimeError("DAPLink 尚未连接")
        for address, value in zip(self.channel_addresses, values):
            self.target.write32(address, max(0, min(100, int(value))))

    def read_values(self) -> tuple[int, int, int]:
        if self.target is None:
            raise RuntimeError("DAPLink 尚未连接")
        return tuple(int(self.target.read32(address)) for address in self.channel_addresses)


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("SP01 RGB 白色调节工具")
        self.resize(720, 470)
        self.writer: Optional[SwdRgbWriter] = None
        self._updating_controls = False

        self.status_label = QLabel("未连接。请先烧录 RGB_TUNER_TEST001，然后连接 DAPLink。")
        self.status_label.setWordWrap(True)
        self.values_label = QLabel("当前占空比：R 70% / G 60% / B 30%")
        self.values_label.setStyleSheet("font-size: 17px; font-weight: 600;")

        self.sliders: list[QSlider] = []
        self.spins: list[QSpinBox] = []
        controls = QGridLayout()
        for row, (name, color) in enumerate(zip(CHANNEL_NAMES, CHANNEL_COLORS)):
            label = QLabel(name)
            label.setStyleSheet(f"color: {color}; font-weight: 700;")
            slider = QSlider()
            slider.setOrientation(Qt.Orientation.Horizontal)
            slider.setRange(0, 100)
            slider.setSingleStep(1)
            slider.setPageStep(5)
            spin = QSpinBox()
            spin.setRange(0, 100)
            spin.setSuffix(" %")
            controls.addWidget(label, row, 0)
            controls.addWidget(slider, row, 1)
            controls.addWidget(spin, row, 2)
            self.sliders.append(slider)
            self.spins.append(spin)
            slider.valueChanged.connect(lambda value, i=row: self.set_channel(i, value))
            spin.valueChanged.connect(lambda value, i=row: self.set_channel(i, value))

        self.connect_button = QPushButton("连接 DAPLink")
        self.disconnect_button = QPushButton("断开")
        self.connect_button.clicked.connect(self.connect_probe)
        self.disconnect_button.clicked.connect(self.disconnect_probe)

        self.pending_write = QTimer(self)
        self.pending_write.setSingleShot(True)
        self.pending_write.setInterval(80)
        self.pending_write.timeout.connect(self.write_current_values)

        self.poll_timer = QTimer(self)
        self.poll_timer.setInterval(500)
        self.poll_timer.timeout.connect(self.refresh_values)

        presets = QHBoxLayout()
        preset_buttons = (
            ("降低蓝光：70 / 60 / 30", (70, 60, 30)),
            ("之前参考：70 / 60 / 43", (70, 60, 43)),
            ("三色全开：100 / 100 / 100", (100, 100, 100)),
            ("熄灯", (0, 0, 0)),
        )
        for title, values in preset_buttons:
            button = QPushButton(title)
            button.clicked.connect(lambda _checked=False, v=values: self.set_values(v))
            presets.addWidget(button)

        connection_box = QGroupBox("连接状态")
        connection_layout = QVBoxLayout(connection_box)
        connection_layout.addWidget(self.status_label)
        connection_layout.addWidget(self.connect_button)
        connection_layout.addWidget(self.disconnect_button)

        sliders_box = QGroupBox("三路亮度（每次 1%）")
        sliders_layout = QVBoxLayout(sliders_box)
        sliders_layout.addLayout(controls)
        sliders_layout.addWidget(self.values_label)

        note = QLabel(
            "本工具只调 RGB，不启动水泵、雷达或蜂鸣器。红/绿/蓝分别对应 PB3/PA5/PA1。\n"
            "先从“降低蓝光”档开始，再一点点拖蓝色滑块；滑块调整约 0.1 秒后生效。"
        )
        note.setWordWrap(True)

        root = QWidget()
        layout = QVBoxLayout(root)
        layout.addWidget(connection_box)
        layout.addWidget(sliders_box)
        layout.addWidget(QLabel("快速档位："))
        layout.addLayout(presets)
        layout.addWidget(note)
        self.setCentralWidget(root)
        self.disconnect_button.setEnabled(False)
        self.set_values((70, 60, 30), write=False)

    def current_values(self) -> tuple[int, int, int]:
        return tuple(slider.value() for slider in self.sliders)

    def set_channel(self, index: int, value: int) -> None:
        if self._updating_controls:
            return
        self._updating_controls = True
        self.sliders[index].setValue(value)
        self.spins[index].setValue(value)
        self._updating_controls = False
        self.update_values_label()
        if self.writer is not None:
            self.pending_write.start()

    def set_values(self, values: tuple[int, int, int], write: bool = True) -> None:
        self._updating_controls = True
        for slider, spin, value in zip(self.sliders, self.spins, values):
            slider.setValue(value)
            spin.setValue(value)
        self._updating_controls = False
        self.update_values_label()
        if write and self.writer is not None:
            self.pending_write.start()

    def update_values_label(self) -> None:
        red, green, blue = self.current_values()
        self.values_label.setText(
            f"当前占空比：R {red}% / G {green}% / B {blue}%"
        )

    def connect_probe(self) -> None:
        try:
            if not DEFAULT_ELF.is_file():
                raise RuntimeError(f"找不到匹配的调色固件 ELF：\n{DEFAULT_ELF}")
            self.writer = SwdRgbWriter(DEFAULT_ELF)
            self.writer.connect()
            self.writer.write_values(self.current_values())
            self.status_label.setText("已连接 DAPLink；滑块变化会实时写入 MCU。")
            self.connect_button.setEnabled(False)
            self.disconnect_button.setEnabled(True)
            self.poll_timer.start()
            self.refresh_values()
        except Exception as exc:
            if self.writer is not None:
                self.writer.close()
            self.writer = None
            self.status_label.setText("连接失败：" + str(exc))
            QMessageBox.critical(self, "连接失败", str(exc))

    def write_current_values(self) -> None:
        if self.writer is None:
            return
        try:
            self.writer.write_values(self.current_values())
            self.status_label.setText("已写入 MCU。保持 DAPLink 连接，可继续调色。")
        except Exception as exc:
            self.status_label.setText("写入失败：" + str(exc))

    def refresh_values(self) -> None:
        if self.writer is None:
            return
        try:
            values = self.writer.read_values()
            self.status_label.setText(
                "DAPLink 已连接；MCU 当前占空比：{} / {} / {}".format(*values)
            )
        except Exception as exc:
            self.status_label.setText("读取失败：" + str(exc))

    def disconnect_probe(self) -> None:
        self.pending_write.stop()
        self.poll_timer.stop()
        if self.writer is not None:
            self.writer.close()
        self.writer = None
        self.connect_button.setEnabled(True)
        self.disconnect_button.setEnabled(False)
        self.status_label.setText("已断开。板上保持最后一次 RGB 设置，断电后恢复默认值。")

    def closeEvent(self, event) -> None:
        self.disconnect_probe()
        event.accept()


def main() -> int:
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
