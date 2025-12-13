# Development Guide

開発者向けドキュメントです。

## プロジェクト構成

```
.
├── src/                        # メインアプリケーション
│   └── main.cpp               # Core0/Core1エントリポイント
├── lib/                        # ライブラリ
│   ├── MotorLogic/            # RPMクランプロジック（テスト可能）
│   ├── SerialProtocol/        # ROS通信プロトコル（テスト可能）
│   ├── PIDController/         # PID制御（テスト可能）【新規】
│   ├── QuadratureEncoder/     # 2相エンコーダ読み取り【新規】
│   ├── MotorDriver/           # PWM+方向出力【新規】
│   ├── MotorController/       # モータ制御統合【新規】
│   ├── ConfigStorage/         # Flash設定保存【新規】
│   └── HardwareConfig/        # ピン定義【新規】
├── test/                       # ユニットテスト
│   ├── test_motor_logic/
│   ├── test_serial_protocol/
│   └── test_pid_controller/   # 【新規】
├── documents/                  # ドキュメント
│   ├── architecture.md        # アーキテクチャ設計
│   ├── development.md         # このファイル
│   ├── test_specifications.md # テスト仕様
│   └── progress.md            # 実装進捗
└── platformio.ini              # PlatformIO設定
```

## 環境構築

### PlatformIOのインストール

```bash
pip install platformio
```

または、VS Codeの拡張機能「PlatformIO IDE」をインストール。

## ビルド

```bash
# Raspberry Pi Pico用にビルド
pio run -e pico

# 生成されるファイル: .pio/build/pico/firmware.uf2
```

## テスト（TDD）

```bash
# 全ユニットテストを実行（PC上で実行）
pio test -e native

# 特定ライブラリのテストのみ実行
pio test -e native -f test_pid_controller
pio test -e native -f test_motor_logic
pio test -e native -f test_serial_protocol
```

テストはPC上で実行され、ハードウェアは不要です。

### TDD開発フロー

1. `documents/test_specifications.md` でテストケースを確認
2. `test/test_<ライブラリ名>/` にテストコードを作成
3. `pio test -e native` でテスト失敗を確認（Red）
4. `lib/<ライブラリ名>/` に実装コードを作成
5. `pio test -e native` でテストパスを確認（Green）
6. 必要に応じてリファクタリング

### テスト対象ライブラリ

| ライブラリ | テスト内容 |
|-----------|-----------|
| MotorLogic | RPMクランプ処理 |
| SerialProtocol | チェックサム計算、バッファ操作 |
| PIDController | PID制御演算（P/I/D各項、出力リミット） |

## 書き込み

```bash
# ビルドして書き込み（Picoを接続した状態で）
pio run -e pico -t upload

# シリアルモニタ
pio device monitor
```

## デバッグ（Debug Probe使用）

Raspberry Pi Debug Probe（公式）を使用したデバッグ環境。

### 接続構成

```
開発PC
├── USB1: Pico本体 ────────> ROS通信（PacketSerial/COBS）
└── USB2: Debug Probe ─────> デバッグログ（Pico Serial1経由）
                      ─────> SWDデバッグ（ブレークポイント等）
```

### 配線

```
Debug Probe              Pico
-----------              ----
[SWD側]
  SWCLK      ─────────>  SWCLK（白リード線マーク側）
  SWDIO      ─────────>  SWDIO
  GND        ─────────>  GND

[UART側]
  UART RX    <─────────  GPIO0（Pico TX = Serial1出力）
  UART TX    ─────────>  GPIO1（Pico RX）
  GND        ─────────>  GND
```

### デバッグビルド

```bash
# デバッグ用ビルド（DEBUG_BUILD有効、最適化なし）
pio run -e pico_debug

# Debug Probe経由で書き込み
pio run -e pico_debug -t upload
```

### デバッグログ確認

PicoのSerial1（GPIO0）から出力されたログが、Debug ProbeのUART経由でPCに届く。

```bash
# ターミナル1: ROS通信（通常通り実行）
ros2 launch your_package robot.launch.py

# ターミナル2: デバッグログ確認（Debug Probeのポート）
pio device monitor --port /dev/cu.usbmodem*   # macOS
pio device monitor --port /dev/ttyACM1        # Linux
```

ポート確認:
```bash
ls /dev/cu.usb*     # macOS
ls /dev/ttyACM*     # Linux
```

### デバッグログの使い方

`DebugLogger.h`を使用。本番ビルド（`pico`環境）では自動的に無効化される。

```cpp
#include "DebugLogger.h"

void setup() {
    DEBUG_INIT();  // Serial1初期化
    DEBUG_PRINTLN("セットアップ開始");
}

void loop() {
    DEBUG_PRINTF("RPM: L=%.2f R=%.2f\n", rpmL, rpmR);
    DEBUG_PID(targetRpm, measuredRpm, output);
    DEBUG_ENCODER(countL, countR, rpmL, rpmR);
}
```

### 利用可能なマクロ

| マクロ | 用途 |
|--------|------|
| `DEBUG_INIT()` | Serial1初期化（setup()内で呼ぶ） |
| `DEBUG_PRINT(msg)` | 改行なし出力 |
| `DEBUG_PRINTLN(msg)` | 改行あり出力 |
| `DEBUG_PRINTF(...)` | フォーマット出力 |
| `DEBUG_INFO/WARN/ERROR(msg)` | ログレベル付き出力 |
| `DEBUG_PID(target, measured, output)` | PID制御デバッグ |
| `DEBUG_ENCODER(cL, cR, rpmL, rpmR)` | エンコーダデバッグ |
| `DEBUG_MOTOR(dutyL, dutyR)` | モータ出力デバッグ |

### SWDデバッグ（ブレークポイント）

VS Code + PlatformIO拡張でブレークポイントデバッグが可能。

```bash
# デバッグセッション開始
pio debug -e pico_debug
```

または、VS Codeの「Run and Debug」からデバッグ開始。

## 設定変更

PIDゲインや最高速度等の設定は、ROS側から `SET_CONFIG` リクエスト（0x04）を送信することでFlashに保存される。
専用の設定ツールは不要で、ROS 2パッケージから直接設定変更が可能。

詳細は `documents/protocol.md` の `0x04: SET_CONFIG` を参照。

## リリース

新しいバージョンをリリースする手順：

```bash
# 1. mainブランチを最新に
git checkout main
git pull

# 2. タグを作成（セマンティックバージョニング）
git tag v1.0.0

# 3. タグをpush
git push --tags
```

タグをpushすると、GitHub Actionsが自動的に：
1. ファームウェアをビルド
2. GitHub Releaseを作成
3. `.uf2`ファイルを添付

## CI/CD

| ワークフロー | トリガー | 内容 |
|-------------|---------|------|
| Test | main へのpush/PR | ユニットテスト + ビルド確認 |
| Release | `v*` タグpush | ファームウェアビルド + Release作成 |

## ライブラリ概要

### 既存ライブラリ

#### MotorLogic
- `clamp_rpm_simple()`: 単純なRPM上限クランプ
- `clamp_rpm_rotation_priority()`: 回転成分を優先したクランプ

#### SerialProtocol
- `calculate_checksum()`: パケットのチェックサム計算
- `write_*_to_buf()` / `read_*_from_buf()`: バッファ操作
- `create_serial_packet()`: パケット生成

### 新規ライブラリ

#### PIDController
- `compute()`: PID制御出力計算
- `setGains()`: ゲイン設定
- `setOutputLimits()`: 出力リミット設定
- `reset()`: 積分値リセット

#### QuadratureEncoder
- `begin()`: 割り込み設定
- `getCount()`: 累積カウント取得
- `getRPM()`: RPM計算

#### MotorDriver
- `begin()`: PWMピン初期化
- `setSpeed()`: 速度設定（-1.0〜1.0）
- `stop()`: 停止

#### MotorController
- `setTargetRPM()`: 目標RPM設定
- `update()`: 制御ループ実行
- `getCurrentRPM_L/R()`: 現在RPM取得
- `getEncoderCount_L/R()`: エンコーダカウント取得

#### ConfigStorage
- `load()`: Flash読み込み
- `save()`: Flash書き込み
- `resetToDefaults()`: 初期値リセット

#### HardwareConfig
- ピンアサイン定数
- PWM周波数定数
- 制御周期定数

## デュアルコア構成

| コア | 処理内容 |
|------|---------|
| Core0 | `setup()` / `loop()`: ROS通信、フェイルセーフ、設定モード |
| Core1 | `setup1()` / `loop1()`: エンコーダ、PID制御、PWM出力 |

コア間データ共有は `SharedMotorData` 構造体 + Mutex。

詳細は `documents/architecture.md` を参照。
