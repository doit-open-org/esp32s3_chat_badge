# ESP32-S3 Chat Badge（电子吧唧 Wi-Fi 款）

[English](README_en.md)

[![ESP-IDF Build](https://github.com/doit-open-org/esp32s3_chat_badge/actions/workflows/build.yml/badge.svg)](https://github.com/doit-open-org/esp32s3_chat_badge/actions/workflows/build.yml)

面向 **MZ46-S3-DZ V1.0** 硬件的开源固件工程，采用 **ESP32-S3 + 360 × 360 圆形触摸屏 + VB6824 音频/离线唤醒 + microSD**。项目基于 `xiaozhi-esp32` 定制，保留应用名 `xiaozhi` 和板型标识 `mz46-s3-dz`，以兼容现有 OTA 元数据、服务端设备识别及量产工具。

> [!IMPORTANT]
> 本仓库包含可编译固件源码，但**不包含 AI 服务端实现、生产凭据和完整的商品化 microSD 素材包**。完成编译和烧录后，语音对话仍需要兼容的 OTA/协议服务；完整产品界面还可能依赖写入 microSD 的素材。

## 项目能力

- Wi-Fi SoftAP 配网，默认热点前缀为 `MZ`
- 360 × 360 JD9855 QSPI LCD 与 CST816D 电容触摸
- VB6824 音频输入、PCM 输出、硬件唤醒词及离线语音命令
- WebSocket 或 MQTT + UDP 的 AI 会话与 Opus 音频传输
- 设备侧 MCP（JSON-RPC 2.0）工具
- microSD 素材索引、图片/动画解码与 USB Mass Storage 模式
- 电池电量、充电状态、背光与空闲功耗管理
- 双 OTA 应用分区、服务发现与自定义 OTA 检查
- 小程序/PSD 多包切换支持

## 硬件基线

| 项目 | 配置 |
|---|---|
| 主控 | ESP32-S3，最高 240 MHz |
| Flash / PSRAM | 16 MB QIO Flash / Octal PSRAM 80 MHz |
| 显示 | JD9855，360 × 360，QSPI |
| 触摸 | CST816D，I²C |
| 音频/唤醒 | VB6824，UART 2 Mbit/s |
| 存储 | microSD，SDMMC 4-bit |
| USB | TinyUSB MSC，用于访问 microSD |
| 硬件版本 | MZ46-S3-DZ V1.0 |

完整引脚表、硬件假设和原理图见 [`docs/hardware/README.md`](docs/hardware/README.md)。

## 开始之前

准备以下内容：

1. **MZ46-S3-DZ V1.0 硬件**。其他 PCB 版本需要先核对引脚、显示、音频、电源和分区配置。
2. **ESP-IDF v5.4.4**。源码声明支持 `>=5.4.0,<5.6.0`，VB6824 组件随仓库提供 ESP-IDF 5.4/5.5 对应预编译库。
3. **Python 3.10+、Git 和串口权限**，或者使用 Docker 构建。
4. **兼容的 AI 后端/OTA 配置服务**。默认流程通常由 OTA 响应下发 MQTT 或 WebSocket 参数；也可在 `menuconfig` 中强制使用固定 WebSocket。
5. **microSD 卡及素材**（需要完整产品 UI、图片、动画或小程序包时）。

更完整的首次编译、烧录和联调步骤见 [`docs/getting-started.md`](docs/getting-started.md)。

## 快速开始

### 1. 克隆仓库

```bash
git clone https://github.com/doit-open-org/esp32s3_chat_badge.git
cd esp32s3_chat_badge
```

### 2. Linux / macOS 编译

```bash
# 按实际 ESP-IDF 安装路径修改
source ~/esp/esp-idf/export.sh

python3 tools/check_repo.py
./tools/build.sh
```

### 3. Windows PowerShell 编译

先打开 ESP-IDF PowerShell，然后执行：

```powershell
python tools/check_repo.py
.\tools\build.ps1
```

### 4. Docker 编译

本机没有 ESP-IDF 环境时：

```bash
./tools/build_docker.sh
```

Docker 和 GitHub Actions 均固定使用 ESP-IDF `v5.4.4`。

### 5. 烧录与监视

Linux：

```bash
./tools/flash.sh /dev/ttyACM0
```

macOS：

```bash
./tools/flash.sh /dev/cu.usbmodemXXXX
```

Windows：

```powershell
idf.py -B build -p COM5 flash monitor
```

退出串口监视器：`Ctrl+]`。

## 首次启动与 Wi-Fi 配网

设备没有已保存的 Wi-Fi 时会进入 SoftAP 配网模式：

1. 在屏幕上确认以 `MZ` 开头的热点名；
2. 使用手机或电脑连接该热点；
3. 打开屏幕显示的配网页面，默认地址为 `http://192.168.4.1`；
4. 选择 2.4 GHz Wi-Fi 并保存；
5. 设备联网后继续执行 OTA/协议配置和 AI 服务连接。

仅完成 Wi-Fi 配网不代表 AI 对话服务已经可用。若设备提示找不到服务器，请检查下一节所述的后端配置。

## AI 服务与协议配置

默认启动流程会先请求 OTA/设备配置接口，然后根据响应选择通信方式：

1. OTA 响应包含 `mqtt`：使用 MQTT 控制消息和 AES-CTR UDP Opus 音频；
2. 否则响应包含 `websocket`：使用 WebSocket JSON 与二进制 Opus 音频；
3. 两者均不存在：固件回退到 MQTT，但仍需要有效的 MQTT 参数；
4. 启用 `CONFIG_CONNECTION_FORCE_FIXED_WEBSOCKET`：跳过动态选择，使用编译时 WebSocket URL/Token。

配置入口：

```bash
idf.py -B build menuconfig
```

重点检查 **Xiaozhi Assistant** 下的 OTA 地址、固定 WebSocket、访问令牌、语言、配网方式和音频选项。仓库不提交 Wi-Fi 密码、生产密钥或私有 Token。

协议细节：

- [中文协议说明](docs/protocol/ESP32S3_CHAT_BADGE_PROTOCOL_CN.md)
- [English protocol specification](docs/protocol/ESP32S3_CHAT_BADGE_PROTOCOL_EN.md)
- [仓库内可见网络端点](docs/network-endpoints.md)

> [!WARNING]
> 为兼容旧固件，默认自定义 OTA 配置仍可能使用 HTTP。公网或量产部署必须改为受控的 HTTPS 服务，并建立固件签名、凭据轮换和服务端鉴权策略。

## 编译产物

默认输出位于 `build/`：

| 文件 | 用途 |
|---|---|
| `build/xiaozhi.bin` | 应用固件 |
| `build/bootloader/bootloader.bin` | Bootloader |
| `build/partition_table/partition-table.bin` | 分区表 |
| `build/flasher_args.json` | 烧录参数 |
| `build/merged-binary.bin` | 执行 `./tools/merge_bin.sh` 后生成的整包固件 |
| `build/merged-binary.bin.sha256` | 整包固件 SHA-256 |

生成整包固件：

```bash
./tools/merge_bin.sh
```

## microSD 与 USB Mass Storage

固件正常运行时由 ESP32-S3 挂载 microSD，并扫描图片、动画和小程序素材。进入 USB MSC 模式后，固件会暂停自身文件系统访问、卸载 VFS，再把 microSD 交给 USB 主机；退出后重新挂载并扫描素材，避免主机和设备同时写卡。

当前产品逻辑通过 BOOT/模式按键的长按事件尝试切换 USB MSC。只有检测到 USB 已连接且 microSD 可用时才会导出磁盘。

仓库不包含完整商品化素材。相关目录和资源格式以当前源码及你的 microSD 素材包为准。

## Flash 分区

16 MB Flash 使用固定布局：

| 分区 | 偏移 | 大小 |
|---|---:|---:|
| NVS | `0x9000` | 16 KB |
| OTA data | `0xD000` | 8 KB |
| PHY init | `0xF000` | 4 KB |
| model / PSD | `0x10000` | 960 KB |
| OTA 0 | `0x100000` | 6 MB |
| OTA 1 | `0x700000` | 6 MB |

修改分区表后必须整片擦除并重新烧录，旧 OTA 包也不再兼容。

## 文档导航

| 文档 | 中文 | English |
|---|---|---|
| 快速入门 | [开始使用](docs/getting-started.md) | [Getting started](docs/getting-started_en.md) |
| 文档索引 | [Documentation](docs/README.md) | [Documentation](docs/README.md) |
| 硬件与引脚 | [硬件参考](docs/hardware/README.md) | [Hardware reference](docs/hardware/README.md) |
| 固件架构 | [架构说明](docs/architecture.md) | [Architecture](docs/architecture.md) |
| 通信协议 | [中文](docs/protocol/ESP32S3_CHAT_BADGE_PROTOCOL_CN.md) | [English](docs/protocol/ESP32S3_CHAT_BADGE_PROTOCOL_EN.md) |
| 网络端点 | [端点清单](docs/network-endpoints.md) | [Endpoint inventory](docs/network-endpoints.md) |
| 发布流程 | [发布说明](docs/releasing.md) | [Release guide](docs/releasing.md) |
| 贡献指南 | [CONTRIBUTING.md](CONTRIBUTING.md) | [CONTRIBUTING.md](CONTRIBUTING.md) |
| 安全策略 | [SECURITY.md](SECURITY.md) | [SECURITY.md](SECURITY.md) |
| 第三方许可 | [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) | [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) |

## 仓库结构

```text
components/                       固定版本或定制的本地组件
main/                             应用、UI、音频、协议和服务
main/boards/esp32s3-chat-badge/   本产品板级代码
main/assets/                      通用语言和提示音资源
main/assets_overrides/            电子吧唧定制资源
partitions/                       16 MB 分区表
scripts/                          资源生成脚本
tools/                            检查、编译、烧录和固件合并工具
docs/                             硬件、协议、架构和发布文档
.github/                          GitHub Actions 与 Issue/PR 模板
```

## 兼容性与已知边界

- 当前开源目标硬件仅为 **MZ46-S3-DZ V1.0**。
- `xiaozhi`、`mz46-s3-dz`、分区布局和 NVS 键名均属于兼容性接口，修改前应评估 OTA、服务端和量产工具影响。
- VB6824 的公开 UART 帧已在协议文档中说明；其预编译库内部实现和原始 OTA 数据流无法仅凭公开源码完整定义。
- WebSocket/MQTT 协议是由当前固件实现反推的兼容说明，不代表独立的正式行业标准。
- 当前 `hello` 声明 60 ms 音频帧，但 VB6824 默认输入是 40-byte/20 ms Opus 帧；公开源码未显式聚合三帧，第三方后端必须做实机音频兼容验证。
- CI 可以验证编译，但显示、触摸、音频、microSD、USB MSC、电池和 OTA 仍需在真实硬件上回归。

## 贡献与发布

提交改动前执行：

```bash
python3 tools/check_repo.py
./tools/build.sh
./tools/merge_bin.sh
```

请阅读 [`CONTRIBUTING.md`](CONTRIBUTING.md)。正式发布 Tag 前，按 [`docs/releasing.md`](docs/releasing.md) 完成硬件回归、固件校验和 Release 资料整理。

## 许可证与上游

本仓库是混合许可证工程：

- 主体及原 `xiaozhi-esp32` 代码按根目录 [`LICENSE`](LICENSE) 中的 MIT License 分发；
- `main/display/page_manager/` 保留 GPL-3.0 License；
- ESP-IDF 组件、VB6824 预编译库、字体、音频、图片和其他素材可能适用各自许可证或授权条件。

分发二进制固件前必须阅读 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。项目基于 [78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) 开发，并使用 Espressif ESP-IDF、LVGL 及多个 ESP Component Registry 组件。

## 联系我们

技术交流、硬件定制或商业合作：

- Email: [lihonggang@doit.am](mailto:lihonggang@doit.am)

微信：

![微信联系方式](docs/wechat_qrcode.jpg)

欢迎交流 ESP32、AI 硬件和嵌入式系统开发。
