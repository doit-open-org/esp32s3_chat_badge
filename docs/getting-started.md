# ESP32-S3 Chat Badge 开始使用

[English](getting-started_en.md) · [返回项目主页](../README.md)

本文面向第一次编译和调试 **MZ46-S3-DZ V1.0** 的开发者。所有命令均从仓库根目录执行。

## 1. 确认交付边界

仓库提供：

- ESP32-S3 固件源码；
- 板级驱动、UI、音频、协议、OTA 和 microSD/USB MSC 功能；
- ESP-IDF 构建脚本和 GitHub Actions；
- 硬件、协议及发布文档。

仓库不提供：

- 可独立部署的 AI 云端服务；
- 生产账号、Token、设备密钥或签名密钥；
- 完整商品化 microSD 素材包；
- VB6824 预编译库内部源码和完整原始 OTA 私有协议。

因此，“编译成功”和“AI 对话可用”是两个独立验收项。

## 2. 准备硬件

- MZ46-S3-DZ V1.0 主板；
- 稳定 USB 数据线和足够电流的 USB 供电；
- microSD 卡（需要素材或 USB MSC 时）；
- 可连接 2.4 GHz Wi-Fi 的网络；
- 串口/USB 设备访问权限。

引脚和原理图见 [`hardware/README.md`](hardware/README.md)。

## 3. 安装开发环境

推荐 ESP-IDF `v5.4.4`。源码允许 5.4.x–5.5.x，但应优先使用 CI 相同版本，降低依赖和 VB6824 ABI 差异。

### Linux / macOS

按 Espressif 官方方式安装 ESP-IDF 后，在每个新终端执行：

```bash
source ~/esp/esp-idf/export.sh
idf.py --version
```

预期版本包含 `v5.4.4`。

### Windows

使用 ESP-IDF 安装器安装 `v5.4.4`，然后打开 **ESP-IDF PowerShell**。不要在未初始化 ESP-IDF 环境的普通 PowerShell 中直接编译。

### Docker

安装 Docker Desktop 或 Docker Engine 后，可跳过本地 ESP-IDF 安装并使用：

```bash
./tools/build_docker.sh
```

## 4. 获取源码

```bash
git clone https://github.com/doit-open-org/esp32s3_chat_badge.git
cd esp32s3_chat_badge
git submodule status
```

本项目依赖 ESP-IDF Component Manager，不需要额外初始化 Git 子模块；最后一条命令没有输出也属于正常情况。

## 5. 运行静态检查

```bash
python3 tools/check_repo.py
```

检查内容包括：

- 必要板级文件是否存在；
- CMake 引用是否可解析；
- 16 MB 分区是否对齐且无重叠；
- 是否混入内部路径、常见 Token、私钥或明文密码日志；
- 是否重新引入无关板型或旧组件。

## 6. 编译

Linux / macOS：

```bash
./tools/build.sh
```

Windows：

```powershell
.\tools\build.ps1
```

首次编译会从 ESP Component Registry 下载托管组件。网络受限时，应先配置代理或镜像，而不是把 `managed_components/` 直接提交到仓库。

成功后至少应存在：

```text
build/xiaozhi.bin
build/bootloader/bootloader.bin
build/partition_table/partition-table.bin
build/flasher_args.json
```

生成完整烧录镜像：

```bash
./tools/merge_bin.sh
```

## 7. 烧录

先确认端口：

```bash
# Linux
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null

# macOS
ls /dev/cu.usbmodem* /dev/cu.usbserial* 2>/dev/null
```

烧录并打开串口监视器：

```bash
./tools/flash.sh /dev/ttyACM0
```

macOS 示例：

```bash
./tools/flash.sh /dev/cu.usbmodemXXXX
```

Windows 示例：

```powershell
idf.py -B build -p COM5 flash monitor
```

首次烧录或分区表发生变化时，建议先整片擦除：

```bash
idf.py -B build -p /dev/ttyACM0 erase-flash
./tools/flash.sh /dev/ttyACM0
```

整片擦除会删除 Wi-Fi、NVS 和 OTA 状态。

## 8. 首次 Wi-Fi 配网

无 Wi-Fi 凭据时，设备会进入 SoftAP 配网：

1. 屏幕显示以 `MZ` 开头的 SSID；
2. 手机或电脑连接该热点；
3. 打开 `http://192.168.4.1`；
4. 选择 2.4 GHz Wi-Fi，输入密码并保存；
5. 观察串口日志，确认获取 IP 并继续请求 OTA/配置服务。

如果页面没有自动弹出，请手动输入上述地址。若热点不存在，先确认屏幕、供电、启动日志和 Wi-Fi 配置模式。

## 9. 配置 AI 服务

### 动态配置（默认）

固件请求 `CONFIG_OTA_URL`，服务端响应可下发：

- `mqtt`：MQTT 连接信息及发布主题；
- `websocket`：WebSocket URL、Token 和协议版本；
- `server_time`：服务器时间；
- `firmware`：可选升级版本和下载 URL。

固件优先使用 MQTT；没有 MQTT 但存在 WebSocket 时使用 WebSocket。

### 固定 WebSocket

```bash
idf.py -B build menuconfig
```

启用：

```text
Xiaozhi Assistant
└── Force fixed WebSocket connection
```

然后填写 WebSocket URL 和访问 Token，重新编译并烧录。Token 未包含认证方案时，固件会自动增加 `Bearer ` 前缀。

### 服务端最小兼容要求

- 在 10 秒内响应客户端 `hello`；
- 返回正确的 `transport`、`session_id` 和 `audio_params`；
- 接收设备 Opus 音频并发送设备支持的控制消息和音频；
- 正确处理 `listen`、`abort`、`mcp` 和会话关闭；
- 输出采样率应尽量匹配设备 16 kHz PCM 播放路径，避免额外重采样失真。

完整字段见 [`protocol/ESP32S3_CHAT_BADGE_PROTOCOL_CN.md`](protocol/ESP32S3_CHAT_BADGE_PROTOCOL_CN.md)。

## 10. 准备 microSD 素材

仓库不附带完整商品素材。需要图片、动画或小程序时：

1. 使用兼容文件系统格式化 microSD；
2. 按当前产品素材包的目录结构复制文件；
3. 断电插卡后重新启动；
4. 观察日志中的挂载和素材扫描结果；
5. 需要电脑访问时，连接 USB 并长按 BOOT/模式按键进入 USB MSC。

USB MSC 模式中不要让设备端和电脑端同时写卡。安全弹出磁盘后再退出 USB 模式或断电。

## 11. 最小验收清单

- [ ] `python3 tools/check_repo.py` 通过；
- [ ] ESP-IDF v5.4.4 完整编译通过；
- [ ] Bootloader、分区表和应用均可烧录；
- [ ] 串口显示 `mz46-s3-dz/2.0.1` 或当前发布版本；
- [ ] LCD、触摸、背光正常；
- [ ] SoftAP 配网及 2.4 GHz Wi-Fi 正常；
- [ ] VB6824 输入、输出、音量、唤醒词正常；
- [ ] WebSocket 或 MQTT/UDP 会话正常；
- [ ] microSD 挂载、素材扫描和 USB MSC 正常；
- [ ] 电池电量、充电状态和休眠行为正常；
- [ ] OTA 在测试分区完成升级及回滚验证。

## 12. 常见问题

### `idf.py: command not found`

当前终端未加载 ESP-IDF：

```bash
source ~/esp/esp-idf/export.sh
```

### VB6824 链接失败或出现 ABI 问题

确认 ESP-IDF 为 5.4.x 或 5.5.x，并优先使用 5.4.4。不要直接升级到未验证的 5.6+。

### 编译成功但 AI 无法使用

依次检查：

1. Wi-Fi 是否已连接；
2. OTA/配置接口是否返回 200 和有效 JSON；
3. MQTT/WebSocket 参数是否写入 NVS；
4. Token、主题、证书和端口是否正确；
5. 服务端是否在 10 秒内返回 `hello`；
6. 服务端音频格式是否与协议一致。

### 屏幕正常但没有产品素材

源码不包含完整商品 microSD 素材包。检查 SD 卡挂载、目录和文件格式。

### USB MSC 不出现磁盘

确认 USB 是数据线、主机已连接、microSD 已成功挂载，然后再长按 BOOT/模式按键。查看串口中 USB 检测、卸载和 TinyUSB 日志。

## 13. 不建议随意修改的兼容项

- CMake 项目名 `xiaozhi`；
- 板型名 `mz46-s3-dz`；
- Flash 分区偏移和大小；
- WebSocket/MQTT NVS 命名空间及键；
- OTA 请求头和响应字段；
- VB6824 UART 命令值。

需要修改时，应同步更新协议、迁移方案、后端、量产工具和旧固件兼容说明。
