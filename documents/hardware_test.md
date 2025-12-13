# 実機テスト手順

ユニットテストではカバーできない部分の実機確認手順。

## 概要

### 確認項目

| Step | 確認内容 | 方法 |
|------|---------|------|
| 0 | ファームウェア書き込み | pio upload |
| 1 | Core0通信 | GET_VERSIONリクエスト |
| 2 | Core1動作 | Debug Probeでログ確認 |
| 3 | エンコーダ | 手動で回してカウント変化確認 |
| 4 | モータ | 低速で回転確認 |

### 必要なもの

- Raspberry Pi Pico
- Debug Probe（推奨、Core1のログ確認用）
- エンコーダ付きDCモータ × 2
- モータドライバ（方向+PWM方式）
- 電源

### 配線

```
Pico                    エンコーダ/モータドライバ
----                    ----------------------
GPIO2  ─────────────>   左エンコーダ A相
GPIO3  ─────────────>   左エンコーダ B相
GPIO4  ─────────────>   右エンコーダ A相
GPIO5  ─────────────>   右エンコーダ B相
GPIO6  ─────────────>   左モータ DIR
GPIO7  ─────────────>   左モータ PWM
GPIO8  ─────────────>   右モータ DIR
GPIO9  ─────────────>   右モータ PWM
GND    ─────────────>   共通GND

Debug Probe             Pico
-----------             ----
SWCLK   ─────────────>  SWCLK
SWDIO   ─────────────>  SWDIO
UART RX <─────────────  GPIO0 (TX)
GND     ─────────────>  GND
```

ピン定義は `lib/HardwareConfig/HardwareConfig.h` を参照。

## 準備

### 依存ライブラリのインストール

```bash
pip install pyserial cobs
```

### ビルドと書き込み

```bash
# デバッグビルド（ログ出力あり）
/opt/homebrew/bin/pio run -e pico_debug -t upload

# または通常ビルド
/opt/homebrew/bin/pio run -e pico -t upload
```

### シリアルポート確認

```bash
# macOS
ls /dev/cu.usb*

# 例:
# /dev/cu.usbmodem1101  ← Pico本体（ROS通信用）
# /dev/cu.usbmodem1201  ← Debug Probe（ログ用）
```

## Step 1: Core0通信確認

### テストスクリプト実行

```bash
python tools/test_protocol.py /dev/cu.usbmodem****
```

`****` は実際のポート番号に置き換え。

### 期待結果

```
Connecting to /dev/cu.usbmodem1101...
Connected!

=== Step 1: GET_VERSION ===
  Version: v0.1.0
  [OK] Core0通信成功

=== Step 2: GET_STATUS ===
  Status: 0x0000
  Error Code: 0x00
  Comm Errors: 0
  Uptime: 1234 ms
  [OK] ステータス取得成功

=== Step 3: GET_CONFIG ===
  PID: Kp=1.0, Ki=0.1, Kd=0.01
  Max RPM: 200.0
  Encoder PPR: 1024
  ...
  [OK] 設定取得成功

=== Step 4: MOTOR_COMMAND (停止) ===
  Encoder L: 0
  Encoder R: 0
  Status: 0x0000
  [OK] MOTOR_COMMAND成功

=== Step 5: GET_DEBUG_OUTPUT ===
  Encoder: L=0, R=0
  Target RPM: L=0.0, R=0.0
  Current RPM: L=0.0, R=0.0
  PWM: L=0.00, R=0.00
  [OK] デバッグ出力取得成功
```

### トラブルシューティング

**応答なしの場合:**
1. シリアルポートが正しいか確認
2. Picoが正しく接続されているか確認
3. ファームウェアが正しく書き込まれているか確認

## Step 2: Core1動作確認（Debug Probe）

### ログ監視

別ターミナルで実行:

```bash
# Debug ProbeのUARTポートを指定
pio device monitor --port /dev/cu.usbmodem**** --baud 115200
```

### 期待結果

```
Core0: Setup complete
Core1: Setup complete
RPM: L=0.0/0.0 R=0.0/0.0
RPM: L=0.0/0.0 R=0.0/0.0
...
```

1秒ごとにRPM情報が出力される（DEBUG_BUILD有効時）。

### トラブルシューティング

**ログが出ない場合:**
1. `pico_debug` 環境でビルドしたか確認
2. Debug Probeの配線を確認（GPIO0 → UART RX）
3. ポートが正しいか確認

## Step 3: エンコーダ確認

### テスト方法

テストスクリプトでStep 6まで進め、エンコーダを手で回す。

```
=== Step 6: エンコーダ変化テスト ===
  エンコーダを手で回してください...
  5秒間カウントを監視します
  [1/10] L=0 (diff=+0), R=0 (diff=+0)
  [2/10] L=124 (diff=+124), R=0 (diff=+0)  ← 左を回した
  [3/10] L=256 (diff=+132), R=0 (diff=+0)
  [4/10] L=256 (diff=+0), R=-89 (diff=-89)  ← 右を逆回転
  ...
```

### 確認ポイント

- 正転でカウント増加、逆転で減少
- 両方のエンコーダが反応する
- PPR設定と実際のパルス数が一致

### トラブルシューティング

**カウントが変化しない:**
1. エンコーダの配線を確認
2. エンコーダの電源（5V/3.3V）を確認
3. HardwareConfigのピン設定を確認

**カウント方向が逆:**
- QuadratureEncoderの反転フラグを調整

## Step 4: モータ確認

### 注意事項

- **必ず低速から始める**（0.05 m/s程度）
- モータが空転できる状態で実施
- 異常があればすぐに停止

### Pythonインタラクティブで実行

```python
from tools.test_protocol import PicoProtocol

# 接続
pico = PicoProtocol('/dev/cu.usbmodem****')

# 状態確認
print(pico.get_debug_output())

# 低速で前進（0.05 m/s）
pico.motor_command(0.05, 0.0)

# 状態確認
print(pico.get_debug_output())

# 停止
pico.motor_command(0.0, 0.0)

# 低速で左旋回
pico.motor_command(0.0, 0.5)

# 停止
pico.motor_command(0.0, 0.0)

# 切断
pico.close()
```

### 確認ポイント

- 前進指令で両モータが同じ方向に回転
- 旋回指令で左右が逆方向に回転
- 停止指令で停止
- エンコーダのカウントが変化

### トラブルシューティング

**モータが回らない:**
1. モータドライバの電源を確認
2. PWM/DIRピンの配線を確認
3. DEBUG_OUTPUTでtarget_rpmが設定されているか確認

**回転方向が逆:**
- MotorDriverの反転フラグを調整

**片方だけ回らない:**
- 該当モータの配線を確認
- ドライバの故障を確認

## 全体確認

### 統合テスト

```python
from tools.test_protocol import PicoProtocol
import time

pico = PicoProtocol('/dev/cu.usbmodem****')

# 10秒間、0.1m/sで前進
print("前進開始...")
for i in range(100):
    result = pico.motor_command(0.1, 0.0)
    print(f"Enc: L={result['encoder_l']:6d}, R={result['encoder_r']:6d}")
    time.sleep(0.1)

# 停止
pico.motor_command(0.0, 0.0)
print("停止")

pico.close()
```

### 確認ポイント

- 10秒間安定して動作
- エンコーダカウントが増加し続ける
- フェイルセーフ（通信停止で500ms後に停止）

## 次のステップ

実機確認が完了したら:

1. 問題があれば修正
2. PIDゲインの調整（必要に応じて）
3. Phase 5: ConfigStorage実装へ進む
