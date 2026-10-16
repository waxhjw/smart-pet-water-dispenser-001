# 公开版验证记录

日期：2026-10-16。来源基线：`bb66b63915236ea06edf1679d3ff34366d78c77a`，profile：`SPEC_TEST007_LOW_WATER_WARNING`。

## 已完成

- 独立目录下运行原有主机检查脚本，41 项测试全部 PASS，返回 0。
- 使用本仓库 SDK 源码与 Arm GNU Toolchain 15.2.Rel1，在 Windows / GNU Make 下构建 007，返回 0。
- 对 `-SpecTest007Retry30s` 的 ELF、HEX、BIN 运行产物检查，全部存在。
- 重新构建的 007 HEX 与原开发基线 HEX **逐字节一致**，SHA-256 为 `019e94238f5bb7c4c6c62f6917861f37b0193bc7a7926f8ac0bb11038c797140`。
- 007 体积：text=20468、data=24、bss=1544 字节；HEX=57714、BIN=20492 字节。体积不等同于已验证最坏情况栈空间。
- 同时构建并校验 `-BenchNoPump`，作为首次接板的强制关泵入口；HEX SHA-256 为 `fe527b00e8221611e841db503c3f096b929f728209095417541eaf8958b1f7cc`。
- 硬件归档完整性、元数据清理、原理图图面和文字检查；硬件导出中的器件、坐标与对象引用不改动。
- 项目固件源码与提取时清单一致；公开版只调整构建脚本工具查找，新增说明文档与公开许可。

结构化检查结果见 [publication-checks.json](publication-checks.json)，源码来源见 [source-manifest.json](source-manifest.json)，硬件文件来源见 [hardware-manifest.json](hardware-manifest.json)。

## 构建警告和未完成项

- 链接器仍提示 newlib/nosys 的 `_close/_lseek/_read/_write` 系统调用未实现，以及 ELF LOAD 段含 RWX 权限。已如实记录，不能宣称构建零警告；本次未为消除警告改动底层或链接布局。
- 本次未烧录硬件，未重新验证灯色、水位阈值、供电切换、电池保护、温升、寿命或工厂测试。
- 未在 EDA 软件中重新导入 PCB 工程进行规则/连通性检查。
- ADC 和 RGB 上位机完成静态语法检查，未在本次发布任务中进行 GUI 与硬件连接验证。
- 公开测试快照应按 [分段测试方法](test-plan.md)完成整机回归，具体限制见 [known-limitations.md](known-limitations.md)。
