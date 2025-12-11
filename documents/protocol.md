# 通信プロトコル仕様

ROS 2とRaspberry Pi Pico間のシリアル通信プロトコル仕様。

## 概要

### 通信方式

- **物理層**: USB CDC (仮想シリアルポート)
- **ボーレート**: 115200 bps
- **パケットエンコーディング**: COBS (PacketSerialライブラリ使用)
- **通信モデル**: リクエスト・レスポンス型

### アーキテクチャ

```
ROS 2 (Master)                  Pico (Slave)
    │                               │
    │  [リクエスト]                  │
    │ ─────────────────────────────>│
    │                               │ 処理
    │  [レスポンス]                  │
    │ <─────────────────────────────│
    │                               │
```

- **Picoは常にスレーブ**: 自発的に送信しない
- **ROSが通信タイミングを制御**: 通常100Hzでリクエスト送信
- **リクエストなし時**: モータ制御ループのみ継続

### 通信量

```
MOTOR_COMMAND 100Hz時:
  リクエスト: 12バイト × 100Hz = 1,200 B/s
  レスポンス: 14バイト × 100Hz = 1,400 B/s
  合計: 約26,000 bps (ボーレートの22.6%)
```

## パケット構造

### 共通ヘッダ

**リクエスト（ROS→Pico）**
```
オフセット  サイズ  内容
0          1      request_type
1          1      payload_length
2          2      checksum (payload部のみ、リトルエンディアン)
4          N      payload (可変長)
```

**レスポンス（Pico→ROS）**
```
オフセット  サイズ  内容
0          1      response_type (= request_type)
1          1      payload_length
2          2      checksum (payload部のみ、リトルエンディアン)
4          N      payload (可変長)
```

### チェックサム計算

payload部分のみを対象とした16bit加算チェックサム。

```cpp
uint16_t calculate_checksum(const uint8_t* data, size_t size) {
    uint16_t sum = 0;
    for (size_t i = 0; i < size; i++) {
        sum += data[i];
    }
    return sum;
}
```

### バイトオーダー

すべてのマルチバイト値は**リトルエンディアン**。

## リクエストタイプ一覧

| Type | 名前 | 用途 | v1.0 |
|------|------|------|------|
| 0x00 | MOTOR_COMMAND | 速度指令・エンコーダ取得 | ✅ |
| 0x01 | GET_VERSION | ファームウェアバージョン取得 | ✅ |
| 0x02 | GET_STATUS | 詳細ステータス取得 | ✅ |
| 0x03 | GET_CONFIG | 現在の設定値取得 | ✅ |
| 0x04 | SET_CONFIG | 設定値書き込み（Flash保存） | ✅ |
| 0x05 | GET_DEBUG_OUTPUT | デバッグ用詳細出力 | ✅ |
| 0xFF | RESET | ソフトウェアリセット | ❌ |

## ステータスフラグ定義

16bitのビットフラグ。MOTOR_COMMANDレスポンスおよびGET_STATUSレスポンスで使用。

```
bit 0:  FAILSAFE        - フェイルセーフ発動中（通信途絶で停止）
bit 1:  ENCODER_L_ERROR - 左エンコーダ異常検出
bit 2:  ENCODER_R_ERROR - 右エンコーダ異常検出
bit 3:  MOTOR_L_ERROR   - 左モータ異常検出
bit 4:  MOTOR_R_ERROR   - 右モータ異常検出
bit 5:  CONFIG_EMPTY    - 設定未初期化（Flashにデータなし）
bit 6:  FLASH_ERROR     - Flash読み書きエラー
bit 7:  OVERTEMP        - 過熱検出（将来用）
bit 8:  OVERCURRENT     - 過電流検出（将来用）
bit 9:  LOW_VOLTAGE     - 低電圧検出（将来用）
bit 10-14: reserved     - 予約（将来拡張用）
bit 15: CONFIG_MODE     - 設定モード中
```

**C++定義:**
```cpp
#define STATUS_FAILSAFE        (1 << 0)
#define STATUS_ENCODER_L_ERROR (1 << 1)
#define STATUS_ENCODER_R_ERROR (1 << 2)
#define STATUS_MOTOR_L_ERROR   (1 << 3)
#define STATUS_MOTOR_R_ERROR   (1 << 4)
#define STATUS_CONFIG_EMPTY    (1 << 5)
#define STATUS_FLASH_ERROR     (1 << 6)
#define STATUS_OVERTEMP        (1 << 7)
#define STATUS_OVERCURRENT     (1 << 8)
#define STATUS_LOW_VOLTAGE     (1 << 9)
#define STATUS_CONFIG_MODE     (1 << 15)
```

## 各リクエスト詳細

### 0x00: MOTOR_COMMAND

通常の速度指令とエンコーダカウント取得。100Hzで使用。

**リクエスト: 12バイト**
```
オフセット  サイズ  型       内容
0          1      uint8    request_type = 0x00
1          1      uint8    payload_length = 8
2          2      uint16   checksum
4          4      float    target_rpm_l (左モータ目標RPM)
8          4      float    target_rpm_r (右モータ目標RPM)
```

**レスポンス: 14バイト**
```
オフセット  サイズ  型       内容
0          1      uint8    response_type = 0x00
1          1      uint8    payload_length = 10
2          2      uint16   checksum
4          4      int32    encoder_count_l (左エンコーダ累積カウント)
8          4      int32    encoder_count_r (右エンコーダ累積カウント)
12         2      uint16   status (ステータスフラグ)
```

---

### 0x01: GET_VERSION

ファームウェアバージョンを取得。起動時に1回呼び出す。

**リクエスト: 4バイト**
```
オフセット  サイズ  型       内容
0          1      uint8    request_type = 0x01
1          1      uint8    payload_length = 0
2          2      uint16   checksum = 0
```

**レスポンス: 8バイト**
```
オフセット  サイズ  型       内容
0          1      uint8    response_type = 0x01
1          1      uint8    payload_length = 4
2          2      uint16   checksum
4          1      uint8    version_major
5          1      uint8    version_minor
6          1      uint8    version_patch
7          1      uint8    reserved = 0
```

**バージョン例:** v1.2.3 → major=1, minor=2, patch=3

---

### 0x02: GET_STATUS

詳細ステータスを取得。診断・監視用。

**リクエスト: 4バイト**
```
オフセット  サイズ  型       内容
0          1      uint8    request_type = 0x02
1          1      uint8    payload_length = 0
2          2      uint16   checksum = 0
```

**レスポンス: 16バイト**
```
オフセット  サイズ  型       内容
0          1      uint8    response_type = 0x02
1          1      uint8    payload_length = 12
2          2      uint16   checksum
4          2      uint16   status (ステータスフラグ)
6          1      uint8    error_code (直近のエラーコード)
7          1      uint8    reserved = 0
8          2      uint16   comm_error_count (通信エラー累積)
10         2      uint16   reserved = 0
12         4      uint32   uptime_ms (起動からの経過時間 [ms])
```

**error_code定義:**
```
0x00: NO_ERROR        - エラーなし
0x01: CHECKSUM_ERROR  - チェックサム不一致
0x02: INVALID_COMMAND - 不正なコマンドタイプ
0x03: PAYLOAD_ERROR   - ペイロード長不正
0x10: ENCODER_TIMEOUT - エンコーダ応答なし
0x20: FLASH_ERROR     - Flash読み書きエラー
```

---

### 0x03: GET_CONFIG

現在の設定値を取得。

**リクエスト: 4バイト**
```
オフセット  サイズ  型       内容
0          1      uint8    request_type = 0x03
1          1      uint8    payload_length = 0
2          2      uint16   checksum = 0
```

**レスポンス: 26バイト**
```
オフセット  サイズ  型       内容
0          1      uint8    response_type = 0x03
1          1      uint8    payload_length = 22
2          2      uint16   checksum
4          4      float    pid_kp
8          4      float    pid_ki
12         4      float    pid_kd
16         4      float    max_rpm (モータ最高RPM)
20         2      uint16   encoder_ppr (エンコーダPPR)
22         4      float    gear_ratio (減速比)
```

---

### 0x04: SET_CONFIG

設定値を書き込み、Flashに保存。

**リクエスト: 26バイト**
```
オフセット  サイズ  型       内容
0          1      uint8    request_type = 0x04
1          1      uint8    payload_length = 22
2          2      uint16   checksum
4          4      float    pid_kp
8          4      float    pid_ki
12         4      float    pid_kd
16         4      float    max_rpm
20         2      uint16   encoder_ppr
22         4      float    gear_ratio
```

**レスポンス: 5バイト**
```
オフセット  サイズ  型       内容
0          1      uint8    response_type = 0x04
1          1      uint8    payload_length = 1
2          2      uint16   checksum
4          1      uint8    result (0=成功, その他=エラーコード)
```

**result定義:**
```
0x00: SUCCESS         - 成功
0x01: FLASH_ERROR     - Flash書き込みエラー
0x02: INVALID_VALUE   - 不正な設定値
```

---

### 0x05: GET_DEBUG_OUTPUT

デバッグ用の詳細出力を取得。開発・調整時に使用。

**リクエスト: 4バイト**
```
オフセット  サイズ  型       内容
0          1      uint8    request_type = 0x05
1          1      uint8    payload_length = 0
2          2      uint16   checksum = 0
```

**レスポンス: 28バイト**
```
オフセット  サイズ  型       内容
0          1      uint8    response_type = 0x05
1          1      uint8    payload_length = 24
2          2      uint16   checksum
4          4      int32    encoder_count_l
8          4      int32    encoder_count_r
12         4      float    current_rpm_l (現在のRPM)
16         4      float    current_rpm_r
20         4      float    pwm_duty_l (PWM出力値 -1.0~1.0)
24         4      float    pwm_duty_r
```

---

### 0xFF: RESET（v1.0未実装）

ソフトウェアリセットを実行。将来実装予定。

**リクエスト: 4バイト**
```
オフセット  サイズ  型       内容
0          1      uint8    request_type = 0xFF
1          1      uint8    payload_length = 0
2          2      uint16   checksum = 0
```

**レスポンス: なし**（即座にリセット実行）

---

## フェイルセーフ

### 通信途絶検出

- 500ms間MOTOR_COMMANDを受信しない場合、フェイルセーフ発動
- モータを即座に停止（PWM duty = 0）
- statusのbit 0 (FAILSAFE) をセット

### 復帰条件

- 正常なMOTOR_COMMANDを受信すると自動復帰
- FAILSAFEフラグはクリアされる

---

## 実装例

### Pico側（リクエストハンドラ）

```cpp
void onSerialPacketReceived(const uint8_t* buffer, size_t size) {
    if (size < 4) return;  // 最小ヘッダサイズ

    uint8_t request_type = buffer[0];
    uint8_t payload_length = buffer[1];

    // チェックサム検証
    if (!validateChecksum(buffer, size)) {
        comm_error_count++;
        last_error = ERROR_CHECKSUM;
        return;
    }

    switch (request_type) {
        case 0x00: handleMotorCommand(buffer, size); break;
        case 0x01: handleGetVersion(); break;
        case 0x02: handleGetStatus(); break;
        case 0x03: handleGetConfig(); break;
        case 0x04: handleSetConfig(buffer, size); break;
        case 0x05: handleGetDebugOutput(); break;
        default:
            comm_error_count++;
            last_error = ERROR_INVALID_COMMAND;
            break;
    }
}
```

### ROS側（Python例）

```python
import struct
from cobs import cobs
import serial

class PicoProtocol:
    REQUEST_MOTOR_COMMAND = 0x00
    REQUEST_GET_VERSION = 0x01
    REQUEST_GET_STATUS = 0x02
    REQUEST_GET_CONFIG = 0x03
    REQUEST_SET_CONFIG = 0x04
    REQUEST_GET_DEBUG_OUTPUT = 0x05

    def __init__(self, port, baudrate=115200):
        self.ser = serial.Serial(port, baudrate, timeout=0.1)

    def _calculate_checksum(self, data):
        return sum(data) & 0xFFFF

    def _send_request(self, request_type, payload=b''):
        checksum = self._calculate_checksum(payload)
        header = struct.pack('<BBH', request_type, len(payload), checksum)
        packet = header + payload
        encoded = cobs.encode(packet) + b'\x00'
        self.ser.write(encoded)

    def _receive_response(self):
        data = b''
        while True:
            byte = self.ser.read(1)
            if byte == b'\x00':
                break
            data += byte
        if not data:
            return None
        return cobs.decode(data)

    def motor_command(self, target_rpm_l, target_rpm_r):
        payload = struct.pack('<ff', target_rpm_l, target_rpm_r)
        self._send_request(self.REQUEST_MOTOR_COMMAND, payload)
        response = self._receive_response()
        if response and len(response) >= 14:
            _, _, _, enc_l, enc_r, status = struct.unpack('<BBHiiH', response)
            return enc_l, enc_r, status
        return None

    def get_version(self):
        self._send_request(self.REQUEST_GET_VERSION)
        response = self._receive_response()
        if response and len(response) >= 8:
            _, _, _, major, minor, patch, _ = struct.unpack('<BBHBBBB', response)
            return f"{major}.{minor}.{patch}"
        return None

    def get_config(self):
        self._send_request(self.REQUEST_GET_CONFIG)
        response = self._receive_response()
        if response and len(response) >= 26:
            _, _, _, kp, ki, kd, max_rpm, ppr, gear = struct.unpack('<BBHfffHf', response[0:26])
            return {
                'pid_kp': kp, 'pid_ki': ki, 'pid_kd': kd,
                'max_rpm': max_rpm, 'encoder_ppr': ppr, 'gear_ratio': gear
            }
        return None

    def set_config(self, kp, ki, kd, max_rpm, ppr, gear_ratio):
        payload = struct.pack('<ffffHf', kp, ki, kd, max_rpm, ppr, gear_ratio)
        self._send_request(self.REQUEST_SET_CONFIG, payload)
        response = self._receive_response()
        if response and len(response) >= 5:
            _, _, _, result = struct.unpack('<BBHB', response)
            return result == 0
        return False
```

---

## 拡張性

### 新しいリクエストタイプの追加

1. 未使用のrequest_type番号を割り当て
2. リクエスト・レスポンス構造を定義
3. Pico側にハンドラを追加
4. ROS側に対応メソッドを追加

### パケット長での後方互換性

ROS側はレスポンス長をチェックし、新旧ファームウェアを判別可能。

```python
def get_config(self):
    response = self._receive_response()
    if len(response) >= 26:
        # v1.0: gear_ratio含む
        ...
    elif len(response) >= 22:
        # 旧バージョン: gear_ratioなし
        ...
```

---

## デメリットと対策

| デメリット | 対策 |
|-----------|------|
| 異常通知の遅延（最大10ms） | statusフラグで次回レスポンスに含める |
| ROS停止で通信停止 | フェイルセーフで500ms後にモータ停止 |
| 複数クライアント非対応 | ROS側で中継ノードを作成 |
