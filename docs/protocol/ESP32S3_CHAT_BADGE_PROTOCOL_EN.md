# ESP32-S3 Chat Badge protocol specification (source-inferred)

[中文](ESP32S3_CHAT_BADGE_PROTOCOL_CN.md) · [Back to project home](../../README_en.md)

| Item | Value |
|---|---|
| Document version | 1.1 |
| Firmware snapshot | `xiaozhi` 2.0.1 source tree |
| Target hardware | MZ46-S3-DZ V1.0 |
| Updated | 2026-07-14 |

> [!NOTE]
> This document is a **compatibility specification inferred from the current repository implementation**. It is not an independent normative standard issued by a chip vendor or cloud provider. A backend, app, or coprocessor implementation must ultimately be validated against the source and real hardware. Behavior that exists only in prebuilt libraries is not guessed or presented as a public guarantee.

## 1. Scope

The firmware exposes four groups of interfaces:

1. **AI cloud protocol:** WebSocket, or MQTT control plus UDP audio;
2. **MCP device tools:** JSON-RPC 2.0 carried inside AI cloud JSON messages;
3. **OTA and service discovery:** HTTP(S) JSON for activation, transport configuration, and firmware updates;
4. **Local hardware interfaces:** ESP32-S3 ↔ VB6824 UART frames and a BLE GATT material-download interface.

Primary implementation files:

- [`main/protocols/protocol.*`](../../main/protocols/)
- [`main/protocols/websocket_protocol.cc`](../../main/protocols/websocket_protocol.cc)
- [`main/protocols/mqtt_protocol.cc`](../../main/protocols/mqtt_protocol.cc)
- [`main/application.cc`](../../main/application.cc)
- [`main/mcp_server.*`](../../main/mcp_server.cc)
- [`main/ota.*`](../../main/ota.cc)
- [`main/custom_ota.*`](../../main/custom_ota.cc)
- [`components/vb6824/vb6824.c`](../../components/vb6824/vb6824.c)
- [`components/doit_min_program_upload/ble/gatt_svc.c`](../../components/doit_min_program_upload/ble/gatt_svc.c)

## 2. Architecture

```text
                         HTTP(S) OTA / service discovery
                    +--------------------------------------+
                    |                                      v
+--------------+    |    WebSocket JSON + binary audio   +----------------+
| ESP32-S3     |<---+------------------------------------>| AI backend     |
| Chat Badge   |    |                                      +----------------+
|              |    |    MQTT JSON + encrypted UDP Opus
|              |<---+------------------------------------>
|              |
|              |<------ 2 Mbit/s UART frames -----------> VB6824
|              |
|              |<------ BLE GATT material URL ----------> Mobile app
+--------------+
```

## 3. Common conventions

### 3.1 JSON

- Text messages are UTF-8 JSON;
- `type` is the top-level AI message discriminator;
- Once a session is established, device control messages normally include `session_id`;
- Placeholder values in examples are illustrative and must not be copied into production.

### 3.2 Byte order

Multibyte integers in WebSocket V2/V3 headers and UDP headers use **network byte order (big-endian)**. The VB6824 UART `length` and `command` fields are also transmitted big-endian.

### 3.3 Audio baseline

| Direction | Current firmware baseline |
|---|---|
| Device → AI service | Opus, 16 kHz, mono; client `hello` declares a 60 ms frame duration |
| AI service → device | Opus; server declares sample rate and frame duration in `hello.audio_params` |
| VB6824 → ESP32-S3 | Opus, 16 kHz, 20 ms, currently 40 bytes per frame |
| ESP32-S3 → VB6824 | PCM, 16 kHz, signed 16-bit mono, 10 ms / 320 bytes per block |

> [!IMPORTANT]
> The VB6824 component defaults to 40-byte, 20 ms Opus input frames, while the AI client `hello` declares `frame_duration: 60`. No explicit three-frame aggregation is visible in the public source: each 40-byte result from `InputData()` enters the send queue directly but is labeled as 60 ms. Backend integration must be verified against captured audio and real hardware, and this implementation mismatch should be treated as a compatibility test item.

### 3.4 Timeouts

- Server `hello` deadline after client `hello`: **10 seconds**;
- Audio channel considered stale after **120 seconds** without incoming data;
- MQTT reconnect attempt after a disconnect: approximately **60 seconds**.

## 4. Transport selection and configuration source

Unless `CONFIG_CONNECTION_FORCE_FIXED_WEBSOCKET` is enabled, the device first calls the OTA/configuration endpoint:

1. An `mqtt` object selects MQTT + UDP;
2. Otherwise, a `websocket` object selects WebSocket;
3. If neither exists, the firmware instantiates MQTT as a fallback, but connection still fails without valid MQTT settings.

Dynamic settings are stored in NVS:

| NVS namespace | Keys read by the current implementation |
|---|---|
| `websocket` | `url`, `token`, `version` |
| `mqtt` | `endpoint`, `client_id`, `username`, `password`, `keepalive`, `publish_topic` |
| `wifi` | `ota_url`, which may override the build-time OTA URL |

Fixed WebSocket mode uses `CONFIG_WEBSOCKET_URL` and `CONFIG_WEBSOCKET_ACCESS_TOKEN`. The current fixed branch does not separately override the protocol version, so it uses the code default, WebSocket protocol version **1**, unless the source is changed.

## 5. WebSocket AI protocol

### 5.1 Connection-request headers

| Header | Required | Description |
|---|---:|---|
| `Protocol-Version` | Yes | Binary audio framing version; default `1`, dynamically configurable as `2` or `3` |
| `Device-Id` | Yes | Wi-Fi STA MAC, for example `aa:bb:cc:dd:ee:ff` |
| `Client-Id` | Yes | Persistent UUID v4 generated on first boot |
| `Authorization` | No | Sent when a token is configured; firmware prefixes `Bearer ` when the token contains no space |

### 5.2 Client `hello`

Immediately after connection, the device sends text JSON:

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

Notes:

- `version` must match `Protocol-Version`;
- The current default is actually `1`; the example above illustrates a V3 configuration;
- `features.aec` is also present when `CONFIG_USE_SERVER_AEC` is enabled;
- `mcp` is always advertised as `true` by the current implementation.

### 5.3 Server `hello`

The server must respond within 10 seconds:

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

| Field | Requirement | Description |
|---|---|---|
| `type` | Required | Must be `hello` to enter the handshake parser |
| `transport` | Required | Must be `websocket` |
| `session_id` | Strongly recommended | Used by later control messages; an omitted value leaves an empty string |
| `audio_params.sample_rate` | Optional | Defaults to 24000; 16 kHz is recommended for this board's playback path |
| `audio_params.frame_duration` | Optional | Defaults to 60 ms |

### 5.4 WebSocket binary audio

#### Protocol V1

The WebSocket binary payload is a raw Opus frame with no additional header:

```text
+---------------------------+
| Opus payload, N bytes     |
+---------------------------+
```

#### Protocol V2

```text
Offset  Size  Field          Byte order
0       2     version        big-endian
2       2     type           big-endian
4       4     reserved       zero / reserved
8       4     timestamp      big-endian, milliseconds
12      4     payload_size   big-endian
16      N     payload        Opus when type = 0
```

| `type` | Definition |
|---:|---|
| `0` | Opus audio |
| `1` | Reserved as JSON in the structure comment; not used by the current send path |

#### Protocol V3

```text
Offset  Size  Field          Byte order
0       1     type           0 = Opus
1       1     reserved       0
2       2     payload_size   big-endian
4       N     payload        Opus
```

The server must make `payload_size` consistent with the actual WebSocket frame length. The current parser is designed for a trusted backend and should not be exposed directly to unauthenticated arbitrary binary senders.

## 6. AI control JSON

### 6.1 Device → server

#### Start listening

```json
{
  "session_id": "session-123",
  "type": "listen",
  "state": "start",
  "mode": "auto"
}
```

| `mode` | Meaning |
|---|---|
| `auto` | Automatically stop listening |
| `manual` | Explicit user/device stop |
| `realtime` | Real-time mode; requires compatible server-side AEC support |

#### Stop listening

```json
{
  "session_id": "session-123",
  "type": "listen",
  "state": "stop"
}
```

#### Wake-word detection

```json
{
  "session_id": "session-123",
  "type": "listen",
  "state": "detect",
  "text": "hello xiaozhi"
}
```

#### Abort current speech

```json
{
  "session_id": "session-123",
  "type": "abort",
  "reason": "wake_word_detected"
}
```

`reason` is emitted for wake-word interruption and may be omitted for a generic abort.

#### MCP message

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

When an MQTT audio channel is closed, the device also sends:

```json
{
  "session_id": "session-123",
  "type": "goodbye"
}
```

### 6.2 Server → device

| `type` | Fields | Current behavior |
|---|---|---|
| `tts` | `state: start` | Enter speaking state |
| `tts` | `state: sentence_start`, `text` | Display assistant text |
| `tts` | `state: stop` | End playback and return to idle/listening according to mode |
| `stt` | `text` | Display recognized user text |
| `llm` | `emotion` | Set display emotion |
| `mcp` | object `payload` | Pass to the device MCP parser |
| `system` | `command: reboot` | Schedule a reboot; `reboot` is the only implemented command |
| `alert` | `status`, `message`, `emotion` | Display an alert and play the alert sound |
| `custom` | object `payload` | Only handled when `CONFIG_RECEIVE_CUSTOM_MESSAGE` is enabled |
| `goodbye` | optional `session_id` | MQTT closes the matching session; WebSocket does not have a separate handler for it |

Examples:

```json
{
  "type": "tts",
  "state": "sentence_start",
  "text": "Hello from the badge."
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

The backend must authorize `system.reboot`, MCP firmware-upgrade tools, and other privileged messages. Untrusted user input must never be mapped directly to device-control messages.

## 7. MQTT control + UDP audio

### 7.1 MQTT settings

| Key | Type | Description |
|---|---|---|
| `endpoint` | string | `host` or `host:port`; default port 8883 |
| `client_id` | string | MQTT client ID |
| `username` | string | MQTT username |
| `password` | string | MQTT password/token |
| `keepalive` | number | Seconds, default 240 |
| `publish_topic` | string | Topic used by the device to publish control JSON |

The receive topic is determined by the lower-level MQTT implementation/service setup; this protocol class does not read a separate `subscribe_topic` key.

### 7.2 MQTT client `hello`

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

`features.aec` is present when server-side AEC is enabled.

### 7.3 MQTT server `hello`

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

- `key`: 32 hexadecimal characters, decoded to a 16-byte AES-128 key;
- `nonce`: 32 hexadecimal characters, decoded to a 16-byte initial UDP header/CTR nonce;
- The source does not rigorously validate hex lengths, so the server must provide valid lengths and characters;
- `udp.server` and `udp.port` must be reachable from the device network.

### 7.4 UDP datagram

Each UDP datagram contains a 16-byte plaintext header followed by AES-CTR ciphertext:

```text
Offset  Size  Field          Byte order / meaning
0       1     type           receiver expects 0x01
1       1     flags          supplied by the session nonce/template
2       2     payload_len    big-endian
4       4     ssrc           supplied by the session nonce/template
8       4     timestamp      big-endian
12      4     sequence       big-endian, device starts at 1
16      N     ciphertext     AES-CTR encrypted Opus payload
```

On transmit, the device:

- Copies the 16-byte server-provided `nonce` as a header template;
- Overwrites `payload_len`, `timestamp`, and the incrementing `sequence`;
- Uses that 16-byte header as the AES-CTR counter/nonce;
- Sends the plaintext header followed by ciphertext.

On receive, the device:

- Requires at least 16 bytes and `type == 0x01`;
- Reads timestamp and sequence;
- Discards packets older than the last received sequence and logs gaps;
- Uses the first 16 bytes to decrypt the remainder as Opus.

### 7.5 Security boundary

AES-CTR provides confidentiality but not message integrity or source authentication. The current UDP format has no separate MAC or AEAD tag. A production deployment must at minimum:

- Deliver per-session key/nonce values over an authenticated TLS MQTT channel;
- Use short-lived session keys and prevent nonce reuse;
- Restrict the accepted UDP source at the network or application layer;
- Add replay, tamper, and malformed-packet protection;
- Never reuse the same key + nonce/counter combination.

## 8. MCP device-tool protocol

MCP payloads use JSON-RPC 2.0 and are wrapped in top-level `type: "mcp"` messages.

### 8.1 Request envelope

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

Device requirements:

- `jsonrpc` must be the string `2.0`;
- `method` must be a string;
- `id` must be numeric;
- `params`, when present, must be an object;
- Methods beginning with `notifications` are ignored.

### 8.2 Supported methods

| Method | Description |
|---|---|
| `initialize` | Returns protocol version `2024-11-05`, tool capability, and firmware identity |
| `tools/list` | Lists tools; supports `cursor` and `withUserTools` |
| `tools/call` | Calls a tool using `name` and optional object `arguments` |

A `tools/list` result is limited to approximately 8000 bytes and uses `nextCursor` for pagination. User-only tools are listed only when `withUserTools: true`.

### 8.3 Current common tools

| Tool | Arguments | Description |
|---|---|---|
| `self.get_device_status` | none | Device, volume, screen, battery, and network status |
| `self.audio_speaker.set_volume` | `volume` 0–100 | Set speaker volume |
| `self.screen.set_brightness` | `brightness` 0–100 | Set backlight brightness |
| `self.screen.set_theme` | `theme` | `light` or `dark`, subject to installed themes |
| `self.get_current_version` | none | Board-specific: check and return current version |
| `self.get_new_version` | none | Board-specific: check and return server version |
| `self.upgrade` | none | Board-specific: start the custom OTA check/upgrade flow |

Camera tools are not registered when no camera exists.

### 8.4 Current user-only tools

| Tool | Arguments | Description |
|---|---|---|
| `self.get_system_info` | none | Return full system-information JSON |
| `self.reboot` | none | Reboot the device |
| `self.upgrade_firmware` | `url` | Download an application image from a URL and upgrade |
| `self.screen.get_info` | none | Return display width, height, and monochrome property |
| `self.screen.snapshot` | `url`, `quality` 1–100 (default 80) | Create JPEG screenshot and upload with multipart POST; requires snapshot support |
| `self.screen.preview_image` | `url` | Download and preview an image; requires corresponding LVGL support |
| `self.assets.set_download_url` | `url` | Registered only when the assets partition is valid |

User-only tool descriptions carry:

```json
{
  "annotations": {
    "audience": ["user"]
  }
}
```

### 8.5 Tool call and response

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

A successful result is wrapped as MCP content:

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

The current error shape guarantees only:

```json
{
  "jsonrpc": "2.0",
  "id": 10,
  "error": {
    "message": "Missing valid argument: volume"
  }
}
```

## 9. OTA, activation, and service discovery

### 9.1 Version/configuration request

The device reads NVS `wifi.ota_url` and falls back to `CONFIG_OTA_URL`. It normally sends an HTTP(S) `POST` with a JSON body.

Request headers:

| Header | Value |
|---|---|
| `Activation-Version` | `2` when an eFuse serial number exists, otherwise `1` |
| `Device-Id` | Wi-Fi STA MAC |
| `Client-Id` | Device UUID |
| `Serial-Number` | Present only when an eFuse serial number exists |
| `User-Agent` | `mz46-s3-dz/<firmware-version>` |
| `Accept-Language` | Current firmware language code |
| `Content-Type` | `application/json` |

The system-information body includes:

- Schema version, language, flash size, and minimum free heap;
- MAC, UUID, chip model, and chip information;
- Application name/version, build time, ESP-IDF version, and ELF SHA-256;
- Partition table and running OTA partition;
- Display and board/network information.

### 9.2 Configuration response

Compatible example:

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

Notes:

- String and numeric members of `mqtt` and `websocket` are persisted into their NVS namespaces;
- MQTT has selection priority over WebSocket, so a service normally returns one of them;
- `server_time.timestamp` is milliseconds and `timezone_offset` is minutes; the current implementation adds the offset directly before setting system time;
- `firmware.force == 1` marks an update available even when normal version comparison would not.

### 9.3 Activation

When `activation.challenge` is present and the device has an eFuse serial number, the device posts to `<ota_url>/activate`:

```json
{
  "algorithm": "hmac-sha256",
  "serial_number": "...",
  "challenge": "...",
  "hmac": "64-hex-character-digest"
}
```

The HMAC uses ESP32 HMAC Key0.

| HTTP status | Device interpretation |
|---:|---|
| 200 | Activation succeeded |
| 202 | Not complete yet; treat as pending/timeout |
| Other | Activation failed |

A device without an eFuse serial number cannot produce the HMAC activation payload.

### 9.4 Custom OTA

The board defaults enable a legacy-compatible `CustomOta` check. Response shape:

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

- `force == 1`: mark an update available;
- `forced == 1`: record forced-update policy;
- See [`docs/network-endpoints.md`](../network-endpoints.md) for the actual visible endpoint inventory; replace legacy HTTP services for production.

### 9.5 Firmware download

The device downloads an ESP-IDF application image, writes the next OTA partition, invokes ESP-IDF image validation, selects the new boot partition, and reboots. The current JSON schema has no separate detached-signature or SHA-256 field. Production security should combine HTTPS with signed images, Secure Boot, Flash Encryption, rollback protection, and a controlled release process rather than relying only on the download URL.

## 10. ESP32-S3 ↔ VB6824 UART protocol

### 10.1 Physical settings

| Parameter | Value |
|---|---|
| ESP32-S3 UART | UART1 by default |
| ESP TX → VB RX | GPIO17 |
| ESP RX ← VB TX | GPIO18 |
| Baud rate | 2,000,000 bit/s |
| Format | 8N1 |
| Hardware flow control | None |
| Maximum payload | 512 bytes |

### 10.2 Frame format

```text
Offset  Size  Field       Wire format
0       2     magic       0x55 0xAA
2       2     length      payload length, big-endian
4       2     command     command ID, big-endian
6       N     payload     N = length
6+N     1     checksum    sum(bytes 0..5+N) mod 256
```

- Minimum frame: 7 bytes;
- Maximum frame: 519 bytes;
- The parser searches the stream for `55 AA`, then validates total length and checksum.

### 10.3 Commands

#### VB6824 → ESP32-S3

| Command | Name | Payload |
|---:|---|---|
| `0x2080` | Audio input | Current default: 40-byte Opus 16 kHz/20 ms frame |
| `0x0180` | Control/offline voice command | UTF-8 text; `升级模式` is recognized as an OTA-enter event |
| `0x0280` | Wake-word report | UTF-8 wake-word text |
| `0x0105` | OTA enter/data mode | Enters VB6824 OTA handling; later raw data is processed by a prebuilt OTA library |

#### ESP32-S3 → VB6824

| Command | Name | Payload |
|---:|---|---|
| `0x2081` | Audio output | Current default: PCM 16 kHz, signed 16-bit mono, commonly 320 bytes/10 ms |
| `0x0201` | PCM EOF | Defined; no call observed in the current main path |
| `0x0202` | Control | Defined; no call observed in the current main path |
| `0x0203` | Set volume | 1 byte, 0–31; upper layer maps 0–100 using `floor(value × 31 / 100)` |
| `0x0205` | Request OTA | 1 byte, current request value `0x01` |
| `0x0207` | Request/refresh wake word | 1-byte mode/trigger value |
| `0x0208` | Deep sleep | Empty payload |

### 10.4 VB6824 OTA boundary

The public source defines the framed command that enters OTA and the event handoff. Once OTA mode begins, the byte stream is also passed to a prebuilt `jl_ota` library. The complete raw OTA sequence, packetization, acknowledgements, and error codes cannot be fully derived from the public C source. Third parties should therefore treat VB6824 OTA as implementation-specific and not build production updaters from this section alone.

## 11. BLE material-download protocol (product-specific)

This interface lives in `doit_min_program_upload` and lets a mobile app send an asset URL/path. It is separate from the AI conversation protocol and has a lower stability guarantee than the WebSocket/MQTT interface.

### 11.1 GATT UUIDs

| Role | UUID | Properties |
|---|---|---|
| Service | `6e400001-b5a3-f393-e0a9-e50e24dcca9e` | Primary Service |
| RX (phone → device) | `6e400002-b5a3-f393-e0a9-e50e24dcca9e` | Write / Write Without Response |
| TX (device → phone) | `6e400003-b5a3-f393-e0a9-e50e24dcca9e` | Notify / Read |

### 11.2 Phone write format

RX accepts a UTF-8 string with an internal maximum of 255 bytes. The current payload is **not JSON**. It is one of:

- A complete `https://...` URL;
- A complete `http://...` URL, rewritten to HTTPS by the device;
- A relative path containing a file extension, joined to the product material base URL.

Input without a file extension and without an HTTP(S) scheme fails.

| Extension | Destination/type |
|---|---|
| `.vpg`, `.dat` | Animation / `animal` directory |
| Other valid files | Image / `photo` directory |

### 11.3 Device notifications

TX Notify sends ASCII status text followed by newline `\n`:

| Text | Meaning |
|---|---|
| `0002\n` | 360 × 360 display class; emitted after subscription by the current component build |
| `0001\n` | 240 × 240 class reserved for related boards |
| `0000\n` | 160 × 160 class reserved for related boards |
| `0101\n` | Download and material switch succeeded |
| `0100\n` | Invalid input or download failed |

### 11.4 Typical flow

```text
Phone                                      Badge
  |--- connect GATT ------------------------>|
  |--- subscribe TX notify ----------------->|
  |<-- "0002\n" -----------------------------|
  |--- write HTTPS URL to RX -------------->|
  |                    download + rescan     |
  |<-- "0101\n" or "0100\n" ----------------|
```

Security guidance: allow only trusted HTTPS domains, constrain file size and formats, validate content before decoding, and do not let arbitrary BLE clients trigger unrestricted downloads.

## 12. Compatibility and security requirements

### 12.1 Compatibility surfaces

Changing any of the following may break old firmware, the backend, NVS data, manufacturing tools, or the coprocessor:

- `Protocol-Version` and binary audio headers;
- JSON fields such as `type`, `session_id`, and `audio_params`;
- MQTT/WebSocket NVS keys;
- OTA headers and response fields;
- MCP tool names and arguments;
- VB6824 command IDs, frame length, and checksum;
- BLE UUIDs and four-character status codes.

Evolve the protocol through explicit versions or capability negotiation rather than silently changing existing semantics.

### 12.2 Minimum production security

- Use `wss://` for WebSocket and server-authenticated TLS for MQTT;
- Provision unique, revocable credentials per device;
- Use HTTPS OTA plus signed firmware, Secure Boot, and rollback protection;
- Strictly authorize `system.reboot`, MCP upgrade, and remote-URL tools;
- Never log Wi-Fi passwords, MQTT passwords, tokens, AES keys, or complete activation material;
- Validate JSON, WebSocket binary length, UDP length, BLE URLs, and downloaded files at both service and device boundaries;
- Review [`SECURITY.md`](../../SECURITY.md) and [`docs/network-endpoints.md`](../network-endpoints.md) before release.

## 13. Minimum backend checklist

A minimum WebSocket implementation must:

- [ ] Validate connection headers and device credentials;
- [ ] Complete `hello` within 10 seconds;
- [ ] Parse and generate Opus binary frames for the negotiated version;
- [ ] Handle `listen`, `abort`, and `mcp`;
- [ ] Send `tts`, `stt`, `llm`, and/or audio;
- [ ] Isolate sessions using `session_id`;
- [ ] Handle close, timeout, and reconnect cleanly.

An MQTT/UDP service must additionally:

- [ ] Securely deliver MQTT credentials, UDP address, AES key, and nonce;
- [ ] Maintain UDP sequence and timestamp state;
- [ ] Prevent nonce reuse, replay, and spoofing;
- [ ] Handle `goodbye` when the control session ends.
