# ESP32-S3 Chat Badge（电子吧唧 Wi-Fi 款）

[English](README_en.md)

[![ESP-IDF Build](https://github.com/doit-open-org/esp32s3_chat_badge/actions/workflows/build.yml/badge.svg)](https://github.com/doit-open-org/esp32s3_chat_badge/actions/workflows/build.yml)

这是电子吧唧 Wi-Fi 款的开源固件工程，面向 **ESP32-S3 + 360 × 360 圆屏 + 触摸 + VB6824 音频/离线唤醒 + microSD** 硬件。项目基于 `xiaozhi-esp32` 定制，保留应用名 `xiaozhi` 和板型标识 `mz46-s3-dz`，以兼容现有 OTA、服务端识别及量产固件。

本仓库已经移除原开发目录中的 Git 历史、构建缓存、绝对路径、无关板型、未使用的大体积字体源文件和构建产物；新克隆后只需安装 ESP-IDF，并由 Component Manager 下载其余依赖。

## 功能

- Wi-Fi SoftAP 配网，默认热点前缀为 `MZ`
- 360 × 360 JD9855 QSPI LCD 和 CST816D 电容触摸
- VB6824 音频输入、音频输出及离线语音命令
- XiaoZhi WebSocket/MQTT 协议基础能力与 MCP 工具
- microSD 文件系统、素材扫描和 USB Mass Storage 模式
- 电池电量、充电状态、背光和自动休眠管理
- GIF、QOI/QOIF、JPEG 等界面资源解码
- 双 OTA 分区和自定义 OTA 检查
- 小程序/PSD 多包切换支持（素材存放于 microSD，不在固件分区内）

## 硬件基线

| 项目 | 配置 |
|---|---|
| 主控 | ESP32-S3，240 MHz |
| Flash | 16 MB |
| PSRAM | Octal PSRAM，80 MHz |
| 显示 | JD9855，360 × 360，QSPI |
| 触摸 | CST816D，I²C |
| 音频/唤醒 | VB6824，UART |
| 存储 | microSD，SDMMC 4-bit |
| USB | TinyUSB MSC，用于访问 microSD |
| 硬件版本 | MZ46-S3-DZ V1.0 |

完整引脚表和原理图见 [`docs/hardware/README.md`](docs/hardware/README.md)。

## 开发环境

推荐使用：

- ESP-IDF **v5.4.x**；CI 与 Docker 默认固定为 **v5.4.4**，支持范围为 5.4.x–5.5.x
- Python 3.10 或更高版本
- Git
- Linux、macOS 或 Windows PowerShell

> 上传的历史量产固件 `2.0.1` 是用 ESP-IDF `v5.5.1-dirty` 构建的。当前开源工程保留源码原有的 5.4.1 配置基线，并将 CI 固定到 5.4.4；首次正式发布前应在真实硬件上完成一次全功能回归。

## 快速编译

### Linux / macOS

```bash
git clone https://github.com/doit-open-org/esp32s3_chat_badge.git
cd esp32s3_chat_badge

# 按你的 ESP-IDF 安装位置修改路径
source ~/esp/esp-idf/export.sh

python3 tools/check_repo.py
./tools/build.sh
```

### Windows PowerShell

```powershell
git clone https://github.com/doit-open-org/esp32s3_chat_badge.git
cd esp32s3_chat_badge

# 先在 ESP-IDF PowerShell 环境中执行
python tools/check_repo.py
.\tools\build.ps1
```

没有本地 ESP-IDF 环境时，也可使用 Docker：

```bash
./tools/build_docker.sh
```

首次编译会下载 ESP-IDF Managed Components。生成文件位于 `build/`：

- `build/xiaozhi.bin`：应用固件
- `build/bootloader/bootloader.bin`：Bootloader
- `build/partition_table/partition-table.bin`：分区表
- `build/merged-binary.bin`：执行 `./tools/merge_bin.sh` 后生成的整包固件

> 仓库不包含内部版本中未使用的 LittleFS 示例素材。素材通过配套流程写入 microSD 的 `/sdcard/material/` 等目录。

## 烧录与串口监视

Linux / macOS：

```bash
./tools/flash.sh /dev/cu.usbmodemXXXX
```

Linux 常见端口为 `/dev/ttyACM0` 或 `/dev/ttyUSB0`，macOS 常见端口为 `/dev/cu.usbmodem*`。Windows 可执行：

```powershell
idf.py -B build -p COM5 flash monitor
```

退出串口监视器：`Ctrl+]`。

## 修改配置

构建脚本按以下顺序加载默认配置：

1. `sdkconfig.defaults`
2. `sdkconfig.defaults.esp32s3`
3. `main/boards/esp32s3-chat-badge/sdkconfig.defaults`

进入配置界面：

```bash
idf.py -B build menuconfig
```

重点配置位于 **Xiaozhi Assistant**：

- OTA 地址
- WebSocket 地址和访问令牌
- 配网方式
- 默认语言
- VB6824 OTA
- 音频编码方式

仓库不包含 Wi-Fi 密码、设备密钥或私有访问令牌。默认自定义 OTA 地址使用 HTTP，仅用于保持旧固件行为；量产或公网部署时应替换为受控的 HTTPS 服务，并审查证书校验策略。
仓库内可见的外部服务清单见 [`docs/network-endpoints.md`](docs/network-endpoints.md)。

## microSD 与 USB 模式

固件启动时挂载 microSD，并为界面素材建立索引。长按 BOOT/模式按键可尝试切换 USB Mass Storage 模式；只有在检测到 USB 已连接且 SD 卡挂载成功时才会导出磁盘。进入 USB 模式后，ESP32 端会暂停文件系统访问，退出后重新扫描素材，避免主机和设备同时写卡。

## 分区布局

16 MB Flash 使用固定布局，和上传的历史整包固件一致：

| 分区 | 偏移 | 大小 |
|---|---:|---:|
| NVS | `0x9000` | 16 KB |
| OTA data | `0xD000` | 8 KB |
| PHY init | `0xF000` | 4 KB |
| model / PSD | `0x10000` | 960 KB |
| OTA 0 | `0x100000` | 6 MB |
| OTA 1 | `0x700000` | 6 MB |

修改分区表后，必须整片擦除并重新烧录，旧 OTA 包也不再兼容。

## 仓库结构

```text
components/                       固定版本或定制的本地组件
main/                             应用、UI、音频、协议和服务
main/boards/esp32s3-chat-badge/   本产品板级代码
main/assets/                      通用语言和提示音资源
main/assets_overrides/            电子吧唧定制资源
partitions/                       16 MB 分区表
scripts/                          上游资源生成脚本
tools/                            编译、检查、烧录和合并固件工具
docs/                             硬件、架构和发布文档
```

## 开源发布前检查

正式打 Tag 前执行：

```bash
python3 tools/check_repo.py
./tools/build.sh
./tools/merge_bin.sh
```

然后按 [`docs/releasing.md`](docs/releasing.md) 完成硬件回归、SHA-256 校验和 GitHub Release。品牌图片、音频、字体和 UI 素材应由权利人再次确认可公开分发。

## 贡献

请先阅读 [`CONTRIBUTING.md`](CONTRIBUTING.md)。提交 Issue 时请附上：硬件版本、ESP-IDF 版本、完整编译或运行日志、复现步骤，以及是否改动过 `sdkconfig`。

## 许可证与第三方代码

本仓库是**混合许可证工程**：

- 主体和原 `xiaozhi-esp32` 代码按根目录 [`LICENSE`](LICENSE) 中的 MIT License 分发；
- `main/display/page_manager/` 保留其 GPL-3.0 License；
- ESP-IDF 组件、预编译 VB6824 库、字体、音频和图像资源可能适用各自许可证或授权条件。

分发固件二进制前，请阅读 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。由于 GPL 代码被编入固件，二进制分发的合规义务可能高于单纯 MIT 工程；该说明不是法律意见。

## 致谢

项目基于 [78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) 开发，并使用 Espressif ESP-IDF、LVGL 及多个 ESP Component Registry 组件。感谢所有上游作者和贡献者。
