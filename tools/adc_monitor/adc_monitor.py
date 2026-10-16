from __future__ import annotations

import struct
import sys
from collections import deque
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

try:
    from PySide6.QtCore import QTimer
    from PySide6.QtWidgets import (
        QApplication,
        QComboBox,
        QFileDialog,
        QGridLayout,
        QLabel,
        QLineEdit,
        QMainWindow,
        QMessageBox,
        QPushButton,
        QVBoxLayout,
        QWidget,
    )
except ImportError as exc:
    raise SystemExit(
        "Missing PySide6. Run: python -m pip install -r tools/adc_monitor/requirements.txt"
    ) from exc

try:
    from elftools.elf.elffile import ELFFile
    from pyocd.core.helpers import ConnectHelper
except ImportError as exc:
    raise SystemExit(
        "Missing pyOCD or pyelftools. Run: python -m pip install -r tools/adc_monitor/requirements.txt"
    ) from exc


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_ELF = ROOT / "firmware/target/py32f002b/build-pump-adc-hold-test002/sp01_py32f002b.elf"
CURRENT_SENSE_SHUNT_MILLIOHM = 100
CURRENT_SENSE_GAIN = 20


def estimate_sensed_current_ma(pa3_voltage_mv: float) -> float:
    """Estimate current through R29 from PA3 voltage using the 2026-10-13 schematic."""
    return pa3_voltage_mv * 1000.0 / (
        CURRENT_SENSE_SHUNT_MILLIOHM * CURRENT_SENSE_GAIN
    )


@dataclass(frozen=True)
class ChannelInfo:
    channel: Optional[int]
    label: str


CHANNELS = [
    ChannelInfo(1, "PA3：ADC_IN1 / 通道 1"),
]


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
                    name = symbol.name
                    value = int(symbol.entry["st_value"])
                    if name and value:
                        self.addresses[name] = value

    def require(self, name: str) -> int:
        if name not in self.addresses:
            raise RuntimeError("ELF is missing debug symbol: " + name)
        return self.addresses[name]


class SwdReader:
    def __init__(self, elf_path: Path):
        symbols = ElfSymbols(elf_path)
        self.scan_address = symbols.require("g_sp01_debug_adc_scan_mv")
        self.scan_raw_address = symbols.require("g_sp01_debug_adc_scan_raw")
        self.sequence_address = symbols.require("g_sp01_debug_adc_scan_seq")
        self.selected_channel_address = symbols.require("g_sp01_debug_adc_selected_channel")
        self.selected_raw_address = symbols.require("g_sp01_debug_adc_selected_raw")
        self.selected_mv_address = symbols.require("g_sp01_debug_adc_selected_mv")
        self.selected_average_raw_address = symbols.require(
            "g_sp01_debug_adc_selected_average_raw"
        )
        self.selected_average_mv_address = symbols.require(
            "g_sp01_debug_adc_selected_average_mv"
        )
        self.sample_count_address = symbols.require("g_sp01_debug_adc_sample_count")
        self.discard_count_address = symbols.require("g_sp01_debug_adc_discard_count")
        self.stable_address = symbols.require("g_sp01_debug_adc_stable")
        self.outputs_address = symbols.require("g_sp01_debug_outputs")
        self.uptime_address = symbols.require("g_sp01_debug_uptime_ms")
        self.session = None
        self.target = None

    def connect(self) -> None:
        self.session = ConnectHelper.session_with_chosen_probe(
            return_first=True,
            auto_open=False,
            options={
                "frequency": 100_000,
                "target_override": "cortex_m",
            },
        )
        if self.session is None:
            raise RuntimeError("No DAPLink/CMSIS-DAP probe found")
        self.session.open()
        self.target = self.session.board.target

    def close(self) -> None:
        if self.session is not None:
            self.session.close()
        self.session = None
        self.target = None

    def read_u32(self, address: int) -> int:
        if self.target is None:
            raise RuntimeError("DAPLink is not connected")
        data = bytes(self.target.read_memory_block8(address, 4))
        return struct.unpack("<I", data)[0]

    def read_u16(self, address: int) -> int:
        if self.target is None:
            raise RuntimeError("DAPLink is not connected")
        data = bytes(self.target.read_memory_block8(address, 2))
        return struct.unpack("<H", data)[0]

    def read_snapshot(self) -> dict[str, object]:
        if self.target is None:
            raise RuntimeError("DAPLink is not connected")

        scan_data = bytes(self.target.read_memory_block8(self.scan_address, 16))
        channels_mv = list(struct.unpack("<8H", scan_data))
        scan_raw_data = bytes(self.target.read_memory_block8(self.scan_raw_address, 16))
        channels_raw = list(struct.unpack("<8H", scan_raw_data))

        output_data = bytes(self.target.read_memory_block8(self.outputs_address, 6))

        return {
            "sequence": self.read_u32(self.sequence_address),
            "uptime_ms": self.read_u32(self.uptime_address),
            "channels_mv": channels_mv,
            "channels_raw": channels_raw,
            "selected_channel": self.read_u16(self.selected_channel_address) & 0xFF,
            "selected_raw": self.read_u16(self.selected_raw_address),
            "selected_mv": self.read_u16(self.selected_mv_address),
            "selected_average_raw": self.read_u16(self.selected_average_raw_address),
            "selected_average_mv": self.read_u16(self.selected_average_mv_address),
            "sample_count": self.read_u16(self.sample_count_address),
            "discard_count": self.read_u16(self.discard_count_address),
            "stable": bool(self.read_u16(self.stable_address)),
            "pump_enable": bool(output_data[0]),
        }


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("SP01 ADC DAPLink Monitor")
        self.resize(820, 460)

        self.reader: Optional[SwdReader] = None
        self.adc_history: deque[int] = deque(maxlen=5)
        self.timer = QTimer(self)
        self.timer.setInterval(250)
        self.timer.timeout.connect(self.poll)

        self.elf_edit = QLineEdit(str(DEFAULT_ELF))
        self.channel_combo = QComboBox()

        for info in CHANNELS:
            self.channel_combo.addItem(
                info.label,
                info.channel,
            )
        self.channel_combo.setCurrentIndex(0)

        self.connect_button = QPushButton("Connect DAPLink")
        self.disconnect_button = QPushButton("Disconnect")
        self.choose_button = QPushButton("Choose ELF")

        self.status_label = QLabel("Disconnected")
        self.value_label = QLabel("-- mV")
        self.smoothed_value_label = QLabel("-- mV")
        self.raw_label = QLabel("--")
        self.average_label = QLabel("-- mV / --")
        self.current_estimate_label = QLabel("-- mA")
        self.stability_label = QLabel("--")
        self.pump_label = QLabel("--")
        self.sequence_label = QLabel("--")
        self.uptime_label = QLabel("--")

        self.connect_button.clicked.connect(self.connect_probe)
        self.disconnect_button.clicked.connect(self.disconnect_probe)
        self.choose_button.clicked.connect(self.choose_elf)
        self.channel_combo.currentIndexChanged.connect(self.channel_changed)

        root = QWidget()
        layout = QVBoxLayout(root)

        row_elf = QGridLayout()
        row_elf.addWidget(QLabel("Debug ELF:"), 0, 0)
        row_elf.addWidget(self.elf_edit, 0, 1)
        row_elf.addWidget(self.choose_button, 0, 2)
        row_elf.addWidget(self.connect_button, 1, 0)
        row_elf.addWidget(self.disconnect_button, 1, 1)
        row_elf.addWidget(self.status_label, 1, 2)
        layout.addLayout(row_elf)

        row_channel = QGridLayout()
        row_channel.addWidget(QLabel("测试引脚 / ADC 通道:"), 0, 0)
        row_channel.addWidget(self.channel_combo, 0, 1, 1, 2)
        row_channel.addWidget(QLabel("最新 ADC 值:"), 1, 0)
        row_channel.addWidget(self.value_label, 1, 1)
        row_channel.addWidget(QLabel("Raw code:"), 1, 2)
        row_channel.addWidget(self.raw_label, 1, 3)
        row_channel.addWidget(QLabel("约 1 秒均值:"), 2, 0)
        row_channel.addWidget(self.smoothed_value_label, 2, 1)
        row_channel.addWidget(QLabel("Pump output:"), 2, 2)
        row_channel.addWidget(self.pump_label, 2, 3)
        row_channel.addWidget(QLabel("16 次均值:"), 3, 0)
        row_channel.addWidget(self.average_label, 3, 1)
        row_channel.addWidget(QLabel("Sequence:"), 3, 2)
        row_channel.addWidget(self.sequence_label, 3, 3)
        row_channel.addWidget(QLabel("Uptime:"), 4, 0)
        row_channel.addWidget(self.uptime_label, 4, 1)
        row_channel.addWidget(QLabel("Stability:"), 4, 2)
        row_channel.addWidget(self.stability_label, 4, 3)
        row_channel.addWidget(QLabel("R29 支路电流估算（约 1 秒均值）:"), 5, 0)
        row_channel.addWidget(self.current_estimate_label, 5, 1, 1, 3)
        layout.addLayout(row_channel)

        help_label = QLabel(
            "PA3 对应 ADC_IN1（通道 1）。新原理图按 R29=100mΩ、INA180A1=20倍估算支路电流；"
            "估算值未经仪表校准，只供趋势比较，不能作为低水位判定。\n"
            "必须选择与已烧录 HEX 同目录、同版本的 ELF，否则显示值无效。"
        )
        help_label.setWordWrap(True)
        layout.addWidget(help_label)

        self.setCentralWidget(root)
        self.disconnect_button.setEnabled(False)

    def choose_elf(self) -> None:
        path, _ = QFileDialog.getOpenFileName(
            self,
            "Choose debug ELF",
            str(ROOT),
            "ELF files (*.elf)",
        )
        if path:
            self.elf_edit.setText(path)

    def channel_changed(self, _index: int) -> None:
        self.adc_history.clear()
        self.value_label.setText("-- mV")
        self.smoothed_value_label.setText("-- mV")
        self.raw_label.setText("--")
        self.average_label.setText("-- mV / --")
        self.current_estimate_label.setText("-- mA")
        self.stability_label.setText("--")

    def connect_probe(self) -> None:
        try:
            elf_path = Path(self.elf_edit.text()).expanduser()
            if not elf_path.is_file():
                raise RuntimeError("ELF file not found: " + str(elf_path))

            self.reader = SwdReader(elf_path)
            self.reader.connect()
            self.adc_history.clear()

            self.connect_button.setEnabled(False)
            self.disconnect_button.setEnabled(True)
            self.status_label.setText("DAPLink connected")
            self.timer.start()
        except Exception as exc:
            self.reader = None
            self.status_label.setText("Connection failed")
            QMessageBox.critical(self, "Connection failed", str(exc))

    def disconnect_probe(self) -> None:
        self.timer.stop()
        if self.reader is not None:
            self.reader.close()
        self.reader = None
        self.adc_history.clear()
        self.connect_button.setEnabled(True)
        self.disconnect_button.setEnabled(False)
        self.status_label.setText("Disconnected")

    def poll(self) -> None:
        if self.reader is None:
            return

        try:
            snapshot = self.reader.read_snapshot()
            channel = int(self.channel_combo.currentData())
            value_mv = int(snapshot["channels_mv"][channel])
            value_raw = int(snapshot["channels_raw"][channel])

            self.value_label.setText("{} mV".format(value_mv))
            self.adc_history.append(value_mv)
            smoothed_mv = sum(self.adc_history) / len(self.adc_history)
            self.smoothed_value_label.setText(
                "{:.0f} mV ({} 点)".format(smoothed_mv, len(self.adc_history))
            )
            self.current_estimate_label.setText(
                "{:.0f} mA（粗估）".format(estimate_sensed_current_ma(smoothed_mv))
            )
            self.raw_label.setText(str(value_raw))
            selected_channel = int(snapshot["selected_channel"])
            average_mv = int(snapshot["selected_average_mv"])
            average_raw = int(snapshot["selected_average_raw"])
            self.average_label.setText(
                "CH{}: {} mV / {}".format(selected_channel, average_mv, average_raw)
            )
            self.stability_label.setText(
                "stable (N={})".format(snapshot["sample_count"])
                if snapshot["stable"]
                else "warming up (N={})".format(snapshot["sample_count"])
            )
            self.pump_label.setText(
                "ON" if snapshot["pump_enable"] else "OFF"
            )
            self.sequence_label.setText(str(snapshot["sequence"]))
            self.uptime_label.setText(
                "{:.2f} s".format(int(snapshot["uptime_ms"]) / 1000.0)
            )
        except Exception as exc:
            self.status_label.setText("Read failed: " + str(exc))

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
