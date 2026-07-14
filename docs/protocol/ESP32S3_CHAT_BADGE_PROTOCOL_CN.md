# ESP32-S3 Chat Badge 协议说明（基于源码反推）

[English](ESP32S3_CHAT_BADGE_PROTOCOL_EN.md) · [返回项目主页](../../README.md)

| 项目 | 内容 |
|---|---|
| 文档版本 | 1.1 |
| 对应固件 | `xiaozhi` 2.0.1 源码快照 |
| 目标硬件 | MZ46-S3-DZ V1.0 |
| 更新时间 | 2026-07-14 |

> [!NOTE]
> 本文是根据当前仓库实现反向整理的**兼容性说明**，不是芯片厂商或云服务商发布的独立正式标准。服务端、APP 或协处理器固件应以实际源码和联调结果为最终依据。未在公开源码中实现或仅存在于预编译库中的行为，不作推测性承诺。

## 1. 协议范围

当前固件涉及四类通信接口：

1. **AI 云端协议**：WebSocket，或 MQTT 控制面 + UDP 音频面；
2. **MCP 设备工具协议**：封装在 AI 云端 JSON 消息中的 JSON-RPC 2.0；
3. **OTA/服务发现协议**：HTTP(S) JSON，用于激活、下发传输参数和固件升级；
4. **本地硬件协议**：ESP32-S3 与 VB6824 的 UART 帧，以及用于素材下载的 BLE GATT 接口。

主要实现文件：

- [`main/protocols/protocol.*`](../../main/protocols/)
- [`main/protocols/websocket_protocol.cc`](../../main/protocols/websocket_protocol.cc)
- [`main/protocols/mqtt_protocol.cc`](../../main/protocols/mqtt_protocol.cc)
- [`main/application.cc`](../../main/application.cc)
- [`main/mcp_server.*`](../../main/mcp_server.cc)
- [`main/ota.*`](../../main/ota.cc)
- [`main/custom_ota.*`](../../main/custom_ota.cc)
- [`components/vb6824/vb6824.c`](../../components/vb6824/vb6824.c)
- [`components/doit_min_program_upload/ble/gatt_svc.c`](../../components/doit_min_program_upload/ble/gatt_svc.c)

## 2. 总体架构

```text
                         HTTP(S) OTA / service discovery
                    +--------------------------------------+
                    |                                      v
+--------------+    |    WebSocket JSON + binary audio   +----------------+
| ESP32-S3     |<---+------------------------------------>| AI backend     |
| Chat Badge   |    |                                      +----------------+
|              |    |    MQTT JSON + UDP encrypted Opus
|              |<---+------------------------------------>
|              |
|              |<------ 2 Mbit/s UART frames -----------> VB6824
|              |
|              |<------ BLE GATT material URL ----------> Mobile app
+--------------+
```

## 3. 公共约定

### 3.1 JSON

- 文本消息使用 UTF-8 JSON；
- `type` 是 AI 协议顶层消息类型；
- 会话建立后，设备发送的控制消息通常带 `session_id`；
- JSON 示例中的占位值仅用于说明，不应直接用于生产。

### 3.2 字节序

WebSocket V2/V3 头部以及 UDP 头部中的多字节整数使用**网络字节序（大端）**。VB6824 UART 帧的 `length` 和 `command` 也按大端在线路上传输。

### 3.3 音频基线

| 方向 | 当前固件基线 |
|---|---|
| 设备上传到 AI 服务 | Opus，16 kHz，单声道；客户端 `hello` 声明帧时长 60 ms |
| AI 服务下发到设备 | Opus；服务端在 `hello.audio_params` 中声明采样率和帧时长 |
| VB6824 → ESP32-S3 | Opus，16 kHz，20 ms，当前默认每帧 40 字节 |
| ESP32-S3 → VB6824 | PCM，16 kHz，16-bit，单声道，10 ms，每块 320 字节 |

> [!IMPORTANT]
> 当前 VB6824 组件默认输出 40-byte、20 ms Opus 帧，而 AI 客户端 `hello` 固定声明 `frame_duration: 60`。公开源码中没有看到显式的三帧聚合；每次 `InputData()` 返回的 40-byte 帧会直接进入发送队列，但被标记为 60 ms。后端接入时必须以真实音频帧和实机联调为准，并把这一实现差异视为兼容性测试项。

### 3.4 超时

- 设备发送客户端 `hello` 后等待服务器 `hello`：**10 秒**；
- 音频通道超过 **120 秒**没有收到数据，被视为超时；
- MQTT 断开后，当前实现约 **60 秒**后尝试重连。

## 4. 传输方式选择与配置来源

除非编译时启用 `CONFIG_CONNECTION_FORCE_FIXED_WEBSOCKET`，设备会先调用 OTA/配置接口：

1. 响应含 `mqtt` 对象：选择 MQTT + UDP；
2. 否则响应含 `websocket` 对象：选择 WebSocket；
3. 两者都没有：回退创建 MQTT 协议对象，但没有有效 MQTT 参数时无法连接。

动态配置保存在 NVS：

| NVS 命名空间 | 当前实现读取的键 |
|---|---|
| `websocket` | `url`、`token`、`version` |
| `mqtt` | `endpoint`、`client_id`、`username`、`password`、`keepalive`、`publish_topic` |
| `wifi` | `ota_url`（可覆盖编译时 OTA URL） |

固定 WebSocket 模式读取 `CONFIG_WEBSOCKET_URL` 和 `CONFIG_WEBSOCKET_ACCESS_TOKEN`。当前固定模式没有单独覆盖协议版本，因而使用代码默认的 WebSocket 协议版本 **1**，除非源码另行调整。

## 5. WebSocket AI 协议

### 5.1 连接请求头

| Header | 必选 | 说明 |
|---|---:|---|
| `Protocol-Version` | 是 | 当前音频二进制协议版本，默认 `1`；动态配置可设为 `2` 或 `3` |
| `Device-Id` | 是 | Wi-Fi STA MAC，格式如 `aa:bb:cc:dd:ee:ff` |
| `Client-Id` | 是 | 首次启动生成并保存在 NVS 的 UUID v4 |
| `Authorization` | 否 | 配置 Token 非空时发送；若 Token 中没有空格，固件自动增加 `Bearer ` |

### 5.2 设备 `hello`

连接成功后，设备立即发送文本 JSON：

```json
{
  "type": "hello",
  "version": 3,
  "features": {
    "mcp": true
  },
  "transport": "websocket",
  "audio_params": {
    "format": "opus",
    "sample_rate": 16000,
    "channels": 1,
    "frame_duration": 60
  }
}
```

说明：

- 示例中的 `version` 必须与 `Protocol-Version` 一致；
- 当前默认实际值为 `1`，上例使用 `3` 仅展示 V3 配置；
- 启用 `CONFIG_USE_SERVER_AEC` 时，`features` 还包含 `"aec": true`；
- `mcp` 在当前实现中始终声明为 `true`。

### 5.3 服务器 `hello`

服务器应在 10 秒内返回：

```json
{
  "type": "hello",
  "transport": "websocket",
  "session_id": "session-123",
  "audio_params": {
    "sample_rate": 16000,
    "frame_duration": 60
  }
}
```

字段说明：

| 字段 | 要求 | 说明 |
|---|---|---|
| `type` | 必选 | 必须为 `hello`，否则不会进入握手解析 |
| `transport` | 必选 | 必须为 `websocket` |
| `session_id` | 建议必选 | 后续控制消息携带；缺失时设备使用空字符串 |
| `audio_params.sample_rate` | 可选 | 缺省为 24000；建议与设备 16 kHz 输出路径匹配 |
| `audio_params.frame_duration` | 可选 | 缺省为 60 ms |

### 5.4 WebSocket 二进制音频

#### 协议 V1

WebSocket Binary Payload 直接等于一帧 Opus 数据，无额外头部：

```text
+---------------------------+
| Opus payload, N bytes     |
+---------------------------+
```

#### 协议 V2

```text
Offset  Size  Field          Byte order
0       2     version        big-endian
2       2     type           big-endian
4       4     reserved       zero / reserved
8       4     timestamp      big-endian, milliseconds
12      4     payload_size   big-endian
16      N     payload        Opus when type = 0
```

| `type` | 定义 |
|---:|---|
| `0` | Opus 音频 |
| `1` | 结构体注释预留为 JSON；当前发送路径未使用 |

#### 协议 V3

```text
Offset  Size  Field          Byte order
0       1     type           0 = Opus
1       1     reserved       0
2       2     payload_size   big-endian
4       N     payload        Opus
```

服务端必须保证 `payload_size` 与实际 WebSocket 帧长度一致。当前设备解析路径主要面向可信后端，不应直接暴露给未认证的任意二进制发送方。

## 6. AI 控制 JSON

### 6.1 设备 → 服务器

#### 开始监听

```json
{
  "session_id": "session-123",
  "type": "listen",
  "state": "start",
  "mode": "auto"
}
```

`mode`：

| 值 | 说明 |
|---|---|
| `auto` | 自动停止监听 |
| `manual` | 由用户/设备显式停止 |
| `realtime` | 实时模式，需要服务端 AEC 能力配合 |

#### 停止监听

```json
{
  "session_id": "session-123",
  "type": "listen",
  "state": "stop"
}
```

#### 唤醒词检测

```json
{
  "session_id": "session-123",
  "type": "listen",
  "state": "detect",
  "text": "你好小智"
}
```

#### 中止当前播报

```json
{
  "session_id": "session-123",
  "type": "abort",
  "reason": "wake_word_detected"
}
```

`reason` 仅在唤醒词打断时发送；普通中止可以省略。

#### MCP 消息

```json
{
  "session_id": "session-123",
  "type": "mcp",
  "payload": {
    "jsonrpc": "2.0",
    "id": 1,
    "result": {}
  }
}
```

MQTT 通道关闭时还会发送：

```json
{
  "session_id": "session-123",
  "type": "goodbye"
}
```

### 6.2 服务器 → 设备

| `type` | 字段 | 当前行为 |
|---|---|---|
| `tts` | `state: start` | 进入播报状态 |
| `tts` | `state: sentence_start`, `text` | 在屏幕显示助手文本 |
| `tts` | `state: stop` | 播报结束，按监听模式回到空闲或监听 |
| `stt` | `text` | 在屏幕显示用户识别文本 |
| `llm` | `emotion` | 设置屏幕情绪/表情 |
| `mcp` | 对象 `payload` | 交给设备侧 MCP 解析器 |
| `system` | `command: reboot` | 安排设备重启；当前仅实现 `reboot` |
| `alert` | `status`、`message`、`emotion` | 显示提示并播放提示音 |
| `custom` | 对象 `payload` | 仅在 `CONFIG_RECEIVE_CUSTOM_MESSAGE` 启用时处理 |
| `goodbye` | 可选 `session_id` | MQTT 模式关闭匹配会话；WebSocket 当前未单独处理该类型 |

示例：

```json
{
  "type": "tts",
  "state": "sentence_start",
  "text": "你好，我是电子吧唧。"
}
```

```json
{
  "type": "alert",
  "status": "warning",
  "message": "Network unavailable",
  "emotion": "circle_xmark"
}
```

服务端必须保护 `system.reboot`、MCP 升级工具和其他高权限消息，不能把未鉴权用户输入直接映射为设备控制指令。

## 7. MQTT 控制面 + UDP 音频面

### 7.1 MQTT 连接参数

| 键 | 类型 | 说明 |
|---|---|---|
| `endpoint` | string | `host` 或 `host:port`；未指定端口时默认 8883 |
| `client_id` | string | MQTT Client ID |
| `username` | string | MQTT 用户名 |
| `password` | string | MQTT 密码/Token |
| `keepalive` | number | 秒，默认 240 |
| `publish_topic` | string | 设备发布控制 JSON 的主题 |

接收主题由底层 MQTT 实现/服务配置决定，当前协议类没有单独读取 `subscribe_topic` 键。

### 7.2 MQTT 设备 `hello`

```json
{
  "type": "hello",
  "version": 3,
  "transport": "udp",
  "features": {
    "mcp": true
  },
  "audio_params": {
    "format": "opus",
    "sample_rate": 16000,
    "channels": 1,
    "frame_duration": 60
  }
}
```

启用服务端 AEC 时，`features.aec` 为 `true`。

### 7.3 MQTT 服务器 `hello`

```json
{
  "type": "hello",
  "transport": "udp",
  "session_id": "session-123",
  "audio_params": {
    "sample_rate": 16000,
    "frame_duration": 60
  },
  "udp": {
    "server": "audio.example.com",
    "port": 12345,
    "key": "00112233445566778899aabbccddeeff",
    "nonce": "01000000000000000000000000000000"
  }
}
```

- `key`：32 个十六进制字符，解码为 16-byte AES-128 key；
- `nonce`：32 个十六进制字符，解码为 16-byte 初始 UDP 头/CTR nonce；
- 当前源码未严格验证十六进制长度，服务端必须提供正确长度和字符；
- `udp.server` 和 `udp.port` 必须可从设备网络访问。

### 7.4 UDP 数据报

每个 UDP 数据报由 16-byte 明文头和 AES-CTR 密文组成：

```text
Offset  Size  Field          Byte order / meaning
0       1     type           receiver expects 0x01
1       1     flags          supplied by session nonce/template
2       2     payload_len    big-endian
4       4     ssrc           supplied by session nonce/template
8       4     timestamp      big-endian
12      4     sequence       big-endian, starts from 1 when device sends
16      N     ciphertext     AES-CTR encrypted Opus payload
```

设备发送时：

- 复制服务器下发的 16-byte `nonce` 作为头模板；
- 覆盖 `payload_len`、`timestamp` 和递增的 `sequence`；
- 使用该 16-byte 头作为 AES-CTR counter/nonce；
- 把明文头与密文一起发送。

设备接收时：

- 要求数据报至少 16 字节且 `type == 0x01`；
- 读取时间戳和序列号；
- 丢弃小于已接收序列号的旧包，并对跳号记录警告；
- 使用数据报前 16 字节解密剩余内容为 Opus。

### 7.5 安全边界

AES-CTR 只提供机密性，不提供消息完整性或来源认证。当前 UDP 音频格式没有独立 MAC/AEAD Tag。生产部署必须至少：

- 通过受认证的 TLS MQTT 通道下发会话 key/nonce；
- 使用短时会话密钥并防止重复 nonce；
- 在网络或应用层限制 UDP 来源；
- 对重放、篡改和异常包做额外保护；
- 不复用同一 key + nonce/计数器组合。

## 8. MCP 设备工具协议

MCP 载荷采用 JSON-RPC 2.0，并封装在顶层 `type: "mcp"` 中。

### 8.1 请求封装

```json
{
  "type": "mcp",
  "payload": {
    "jsonrpc": "2.0",
    "id": 1,
    "method": "initialize",
    "params": {
      "capabilities": {}
    }
  }
}
```

设备要求：

- `jsonrpc` 必须为字符串 `2.0`；
- `method` 必须为字符串；
- `id` 必须为数字；
- `params` 若存在必须为对象；
- 以 `notifications` 开头的方法被忽略。

### 8.2 支持的方法

| 方法 | 说明 |
|---|---|
| `initialize` | 返回协议版本 `2024-11-05`、工具能力和设备固件信息 |
| `tools/list` | 列出工具；支持 `cursor` 和 `withUserTools` |
| `tools/call` | 调用工具；参数为 `name` 和可选对象 `arguments` |

`tools/list` 单个结果负载限制约 8000 字节，超出时通过 `nextCursor` 分页。`withUserTools: true` 才会列出标记为仅用户可见的工具。

### 8.3 当前公共工具

| 工具 | 参数 | 说明 |
|---|---|---|
| `self.get_device_status` | 无 | 设备、音量、屏幕、电池、网络等状态 |
| `self.audio_speaker.set_volume` | `volume` 0–100 | 设置音量 |
| `self.screen.set_brightness` | `brightness` 0–100 | 设置背光 |
| `self.screen.set_theme` | `theme` | `light` 或 `dark`，取决于可用主题 |
| `self.get_current_version` | 无 | 本板自定义：检查并返回当前版本 |
| `self.get_new_version` | 无 | 本板自定义：检查并返回服务端新版本 |
| `self.upgrade` | 无 | 本板自定义：触发自定义 OTA 检查/升级 |

相机不存在时不会注册相机工具。

### 8.4 当前 user-only 工具

| 工具 | 参数 | 说明 |
|---|---|---|
| `self.get_system_info` | 无 | 返回完整系统信息 JSON |
| `self.reboot` | 无 | 重启设备 |
| `self.upgrade_firmware` | `url` | 从指定 URL 下载应用镜像并升级 |
| `self.screen.get_info` | 无 | 返回屏幕宽高和单色属性 |
| `self.screen.snapshot` | `url`, `quality` 1–100（默认 80） | 截屏为 JPEG 并 multipart POST 上传；需启用 snapshot |
| `self.screen.preview_image` | `url` | 下载并预览图片；需对应 LVGL 配置 |
| `self.assets.set_download_url` | `url` | 仅 assets 分区有效时注册 |

工具列表中的 user-only 工具带：

```json
{
  "annotations": {
    "audience": ["user"]
  }
}
```

### 8.5 工具调用与返回

```json
{
  "jsonrpc": "2.0",
  "id": 10,
  "method": "tools/call",
  "params": {
    "name": "self.audio_speaker.set_volume",
    "arguments": {
      "volume": 50
    }
  }
}
```

成功响应的 `result` 由设备封装为 MCP content：

```json
{
  "jsonrpc": "2.0",
  "id": 10,
  "result": {
    "content": [
      {
        "type": "text",
        "text": "true"
      }
    ],
    "isError": false
  }
}
```

错误响应当前只保证：

```json
{
  "jsonrpc": "2.0",
  "id": 10,
  "error": {
    "message": "Missing valid argument: volume"
  }
}
```

## 9. OTA、激活与服务发现

### 9.1 版本/配置请求

设备读取 NVS `wifi.ota_url`；为空时使用 `CONFIG_OTA_URL`。通常发送带 JSON Body 的 HTTP(S) `POST`。

请求头：

| Header | 值 |
|---|---|
| `Activation-Version` | 有 eFuse 序列号时 `2`，否则 `1` |
| `Device-Id` | Wi-Fi STA MAC |
| `Client-Id` | 设备 UUID |
| `Serial-Number` | 仅存在 eFuse 序列号时发送 |
| `User-Agent` | `mz46-s3-dz/<firmware-version>` |
| `Accept-Language` | 当前固件语言代码 |
| `Content-Type` | `application/json` |

Body 是设备系统信息，包含：

- 协议版本、语言、Flash 大小、最低空闲堆；
- MAC、UUID、芯片型号和芯片信息；
- 应用名、版本、编译时间、ESP-IDF 版本、ELF SHA-256；
- 分区表和当前 OTA 分区；
- 显示及板级网络信息。

### 9.2 配置响应

兼容响应示例：

```json
{
  "activation": {
    "message": "Activate this device",
    "code": "123456",
    "challenge": "random-challenge",
    "timeout_ms": 30000
  },
  "mqtt": {
    "endpoint": "broker.example.com:8883",
    "client_id": "badge-001",
    "username": "device-user",
    "password": "device-token",
    "keepalive": 240,
    "publish_topic": "devices/badge-001/up"
  },
  "websocket": {
    "url": "wss://example.com/xiaozhi/v1/",
    "token": "device-token",
    "version": 3
  },
  "server_time": {
    "timestamp": 1770000000000,
    "timezone_offset": 480
  },
  "firmware": {
    "version": "2.0.2",
    "url": "https://example.com/firmware/xiaozhi.bin",
    "force": 0
  }
}
```

说明：

- `mqtt` 和 `websocket` 对象中的字符串/数字键会写入对应 NVS 命名空间；
- 当前选择优先级为 MQTT 高于 WebSocket；通常服务端只下发其中一种；
- `server_time.timestamp` 单位为毫秒，`timezone_offset` 单位为分钟；当前实现把偏移直接加到时间戳后设置系统时间；
- `firmware.force == 1` 时，即使版本比较不认为更高，也标记为可升级。

### 9.3 激活

收到 `activation.challenge` 且设备有 eFuse 序列号时，设备向 `<ota_url>/activate` 发送：

```json
{
  "algorithm": "hmac-sha256",
  "serial_number": "...",
  "challenge": "...",
  "hmac": "64-hex-character-digest"
}
```

HMAC 使用 ESP32 HMAC Key0。响应：

| HTTP 状态 | 设备解释 |
|---:|---|
| 200 | 激活成功 |
| 202 | 尚未完成，按超时/轮询流程处理 |
| 其他 | 激活失败 |

没有 eFuse 序列号的设备不会生成有效的 HMAC 激活载荷。

### 9.4 自定义 OTA

板级默认启用兼容旧产品的 `CustomOta` 检查。响应格式：

```json
{
  "firmware": {
    "version": "2.0.2",
    "url": "https://example.com/firmware/xiaozhi.bin",
    "force": 1,
    "forced": 1
  }
}
```

- `force == 1`：标记存在可用升级；
- `forced == 1`：记录为强制升级策略；
- 实际端点见 [`docs/network-endpoints.md`](../network-endpoints.md)，生产版本应替换旧 HTTP 服务。

### 9.5 固件下载

设备通过 URL 下载 ESP-IDF 应用镜像，写入下一 OTA 分区，调用 ESP-IDF 镜像校验并设置启动分区，然后重启。当前 JSON 协议没有独立的 detached signature 或 SHA-256 字段；生产安全应依赖 HTTPS、ESP-IDF Secure Boot/Flash Encryption、签名镜像和受控发布流程，而不是仅依赖 URL。

## 10. ESP32-S3 ↔ VB6824 UART 协议

### 10.1 物理参数

| 参数 | 值 |
|---|---|
| ESP32-S3 UART | 默认 UART1 |
| ESP TX → VB RX | GPIO17 |
| ESP RX ← VB TX | GPIO18 |
| 波特率 | 2,000,000 bit/s |
| 数据格式 | 8N1 |
| 硬件流控 | 无 |
| 最大 Payload | 512 字节 |

### 10.2 帧格式

```text
Offset  Size  Field       Wire format
0       2     magic       0x55 0xAA
2       2     length      payload length, big-endian
4       2     command     command ID, big-endian
6       N     payload     N = length
6+N     1     checksum    sum(bytes 0..5+N) mod 256
```

- 最短帧：7 字节；
- 最大帧：519 字节；
- 接收器会在数据流中搜索 `55 AA` 并校验总长度与 checksum。

### 10.3 命令表

#### VB6824 → ESP32-S3

| Command | 名称 | Payload |
|---:|---|---|
| `0x2080` | 音频输入 | 当前默认 40-byte Opus 16 kHz/20 ms 帧 |
| `0x0180` | 控制/离线语音命令 | UTF-8 文本；当前代码把 `升级模式` 识别为 OTA 进入事件 |
| `0x0280` | 唤醒词上报 | UTF-8 唤醒词文本 |
| `0x0105` | OTA 进入/数据模式 | 触发 VB6824 OTA 状态；后续原始数据由预编译 OTA 库处理 |

#### ESP32-S3 → VB6824

| Command | 名称 | Payload |
|---:|---|---|
| `0x2081` | 音频输出 | 当前默认 PCM 16 kHz、16-bit、单声道；通常 320 bytes/10 ms |
| `0x0201` | PCM EOF | 已定义，当前主路径未见调用 |
| `0x0202` | 控制 | 已定义，当前主路径未见调用 |
| `0x0203` | 设置音量 | 1 byte，范围 0–31；上层 0–100 映射为 `floor(value × 31 / 100)` |
| `0x0205` | 请求 OTA | 1 byte，当前请求值为 `0x01` |
| `0x0207` | 请求/刷新唤醒词 | 1 byte 模式/触发值 |
| `0x0208` | 深度睡眠 | 空 Payload |

### 10.4 VB6824 OTA 边界

公开源码只定义了进入 OTA 的帧命令和事件衔接。进入 OTA 模式后，字节流还会交给 `jl_ota` 预编译库，完整原始 OTA 时序、分包、确认和错误码无法从当前公开 C 源码完整反推。因此第三方应把 VB6824 OTA 视为实现相关接口，不应仅凭本节自行生产升级工具。

## 11. BLE 素材下载协议（产品特定）

该接口位于 `doit_min_program_upload`，用于手机向设备发送素材 URL/路径。它不是 AI 会话协议，稳定性低于 WebSocket/MQTT 主协议。

### 11.1 GATT UUID

| 角色 | UUID | 属性 |
|---|---|---|
| Service | `6e400001-b5a3-f393-e0a9-e50e24dcca9e` | Primary Service |
| RX（手机 → 设备） | `6e400002-b5a3-f393-e0a9-e50e24dcca9e` | Write / Write Without Response |
| TX（设备 → 手机） | `6e400003-b5a3-f393-e0a9-e50e24dcca9e` | Notify / Read |

### 11.2 手机写入格式

RX 接收 UTF-8 字符串，内部缓冲上限 255 字节。当前内容**不是 JSON**，而是以下之一：

- 完整 `https://...` URL；
- 完整 `http://...` URL，设备会改写为 HTTPS；
- 含文件扩展名的相对路径，设备拼接产品素材基础地址。

无扩展名且不是 HTTP(S) URL 的输入返回失败。

扩展名处理：

| 扩展名 | 目录/类型 |
|---|---|
| `.vpg`、`.dat` | 动画/`animal` 目录 |
| 其他有效文件 | 图片/`photo` 目录 |

### 11.3 设备通知

TX Notify 发送 ASCII 状态码并追加换行符 `\n`：

| 文本 | 含义 |
|---|---|
| `0002\n` | 360 × 360 屏幕类别；当前组件编译配置在订阅后发送 |
| `0001\n` | 240 × 240 类别（其他板型预留） |
| `0000\n` | 160 × 160 类别（其他板型预留） |
| `0101\n` | 下载和素材切换成功 |
| `0100\n` | 输入无效或下载失败 |

### 11.4 典型流程

```text
Phone                                      Badge
  |--- connect GATT ------------------------>|
  |--- subscribe TX notify ----------------->|
  |<-- "0002\n" -----------------------------|
  |--- write HTTPS URL to RX -------------->|
  |                    download + rescan     |
  |<-- "0101\n" or "0100\n" ----------------|
```

安全建议：只允许可信的 HTTPS 域名、限制文件大小和格式、验证内容后再解码，并避免让任意 BLE 客户端驱动设备下载不受控资源。

## 12. 兼容性与安全要求

### 12.1 兼容性接口

以下内容一旦修改，可能同时影响旧固件、服务端、NVS、量产工具或协处理器：

- `Protocol-Version` 和二进制音频头；
- `type`、`session_id`、`audio_params` 等 JSON 字段；
- MQTT/WebSocket NVS 键；
- OTA Header 和响应字段；
- MCP 工具名和参数；
- VB6824 命令 ID、帧长度和 checksum；
- BLE UUID 和四位状态码。

建议以新增版本/能力协商方式演进，不要静默改变现有字段语义。

### 12.2 生产安全最低要求

- WebSocket 使用 `wss://`，MQTT 使用带服务端验证的 TLS；
- 每台设备使用独立、可吊销的凭据；
- OTA URL 使用 HTTPS，并启用签名固件、Secure Boot 和回滚保护；
- 后端严格鉴权 `system.reboot`、MCP 升级和远程 URL 下载；
- 不在日志中输出 Wi-Fi 密码、MQTT 密码、Token、AES key 或完整激活材料；
- 对 JSON、WebSocket 二进制长度、UDP 长度、BLE URL 和下载文件进行服务端及设备端边界校验；
- 发布前审查 [`SECURITY.md`](../../SECURITY.md) 和 [`docs/network-endpoints.md`](../network-endpoints.md)。

## 13. 服务端最小实现清单

WebSocket 服务最小实现：

- [ ] 校验连接头和设备凭据；
- [ ] 在 10 秒内完成 `hello`；
- [ ] 按协商版本解析/生成 Opus 二进制帧；
- [ ] 处理 `listen`、`abort` 和 `mcp`；
- [ ] 发送 `tts`、`stt`、`llm` 或音频；
- [ ] 使用 `session_id` 隔离会话；
- [ ] 正确处理关闭、超时和重连。

MQTT/UDP 服务还需要：

- [ ] 安全下发 MQTT 凭据、UDP 地址、AES key 和 nonce；
- [ ] 维护 UDP sequence/timestamp；
- [ ] 防止 nonce 重用、重放和伪造；
- [ ] 在控制通道结束时处理 `goodbye`。
