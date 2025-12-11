# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## プロジェクト概要

Raspberry Pi Pico用ファームウェア。汎用DCモータ差動二輪ロボットをPID制御する。ROS 2パッケージとUSBシリアル通信（PacketSerial/COBS）で連携。

**現在リファクタリング中**: CuGo専用から汎用DCモータ対応へ移行中。

## ドキュメント構成

| ファイル | 内容 |
|---------|------|
| `documents/architecture.md` | アーキテクチャ設計（デュアルコア構成等） |
| `documents/protocol.md` | 通信プロトコル仕様（リクエスト・レスポンス型） |
| `documents/development.md` | 開発ガイド（ビルド・テスト方法） |
| `documents/test_specifications.md` | TDDテストケース仕様 |
| `documents/progress.md` | 実装進捗管理 |

## ビルドコマンド

```bash
# Raspberry Pi Pico用にビルド
pio run -e pico

# ユニットテスト実行（PC上で実行、ハードウェア不要）
pio test -e native

# 特定テストのみ実行
pio test -e native -f test_motor_logic

# ビルドして接続中のPicoに書き込み
pio run -e pico -t upload

# シリアルモニタ（115200 baud）
pio device monitor
```

出力ファイル: `.pio/build/pico/firmware.uf2`

## アーキテクチャ

### デュアルコア構成

| コア | 役割 |
|-----|------|
| Core0 | ROS通信、コマンド解析、フェイルセーフ、設定モード |
| Core1 | エンコーダ読み取り、PID制御、PWM出力（リアルタイム処理） |

コア間はShared Memory + Mutexでデータ共有。

### ライブラリ構成

**既存（変更なし）:**
- **MotorLogic** (`lib/MotorLogic/`) - RPMクランプロジック
- **SerialProtocol** (`lib/SerialProtocol/`) - パケットエンコード/デコード

**新規（実装予定）:**
- **PIDController** - PID制御演算
- **QuadratureEncoder** - 2相エンコーダ読み取り
- **MotorDriver** - PWM + 方向ピン出力
- **MotorController** - モータ制御統合
- **ConfigStorage** - Flash設定保存
- **HardwareConfig** - ピンアサイン定義

**削除予定:**
- **CugoSDK** (`lib/CugoSDK/`) - LD-2専用のため汎用版では不要

### 通信プロトコル

リクエスト・レスポンス型。ROSがマスター、Picoがスレーブ。

| リクエスト | 用途 |
|-----------|------|
| 0x00 MOTOR_COMMAND | 速度指令・エンコーダ取得（100Hz） |
| 0x01 GET_VERSION | バージョン取得 |
| 0x02 GET_STATUS | 詳細ステータス取得 |
| 0x03 GET_CONFIG | 設定値取得 |
| 0x04 SET_CONFIG | 設定値書き込み（Flash保存） |
| 0x05 GET_DEBUG_OUTPUT | デバッグ出力 |

詳細は `documents/protocol.md` を参照。

### 設定項目（Flash保存）

- PIDゲイン (kp, ki, kd)
- 最高RPM
- エンコーダPPR
- 減速比

## 実装順序

1. HardwareConfig - ピン定義
2. PIDController - テスト作成含む（TDD）
3. QuadratureEncoder - エンコーダ読み取り
4. MotorDriver - PWM出力
5. MotorController - 統合（Core1）
6. ConfigStorage - Flash設定
7. main.cpp改修 - デュアルコア化、新プロトコル対応

進捗は `documents/progress.md` を参照。

## テスト（TDD）

Unityフレームワーク使用。`test/` 配下にライブラリ対応のテストを配置:
- `test/test_motor_logic/`
- `test/test_serial_protocol/`
- `test/test_pid_controller/` （新規予定）

テスト仕様は `documents/test_specifications.md` を参照。

### TDD開発ルール

**重要: テスト実行後は必ず結果をユーザーに提示すること**

1. テストコードを書いたら `pio test -e native` を実行
2. 実行結果（成功/失敗、テスト名、エラー内容）を必ずユーザーに提示
3. ユーザーの確認を得てから次のステップに進む

```
【テスト結果報告フォーマット】
実行コマンド: pio test -e native -f <テスト名>
結果: PASSED / FAILED
テスト数: X tests, Y failures

[失敗時]
失敗したテスト:
- test_xxx: 期待値=YYY, 実際=ZZZ

[成功時]
全テストパス ✓
```

この手順を省略せず、毎回必ず結果を確認しながら進めること。

## CI/CD

- **Testワークフロー**: main へのpush/PRで `pio test -e native` と `pio run -e pico` を実行
- **Releaseワークフロー**: `v*` タグで GitHub Releaseに`.uf2`ファイルを添付

## Raspberry Pi Pico固有API

| 機能 | API |
|------|-----|
| デュアルコア | `setup1()` / `loop1()` |
| Mutex | `pico/mutex.h` |
| タイマー割り込み | `add_repeating_timer_us()` |
| GPIO割り込み | `attachInterrupt()` |
| PWM | `analogWriteFreq()` + `analogWrite()` |
| Flash保存 | `EEPROM` または `LittleFS` |

## コーディング規約

### 言語

- **コメント**: 日本語で記載
- **ログ出力**: 日本語で記載
- **ドキュメント**: 日本語

### 命名規則（Arduinoスタイル）

| 対象 | 規則 | 例 |
|------|------|-----|
| クラス・構造体 | パスカルケース | `MotorRpm`, `PidController` |
| 関数・メソッド | キャメルケース | `getCount()`, `setGains()`, `computeOutput()` |
| 変数 | キャメルケース | `targetRpm`, `maxSpeed`, `encoderCount` |
| 定数(#define) | 大文字スネークケース | `SERIAL_HEADER_SIZE`, `MAX_RPM` |
| constexpr定数 | 大文字スネークケース | `PWM_FREQUENCY`, `CONTROL_PERIOD_US` |
| namespace | パスカルケース | `HardwareConfig`, `Defaults` |
| プライベートメンバ | 末尾アンダースコア | `kp_`, `integral_`, `prevError_` |

### コード例

```cpp
// クラス定義
class PidController {
public:
    float compute(float setpoint, float measured, float dt);
    void setGains(float kp, float ki, float kd);
private:
    float kp_;
    float integral_;
};

// 使用例
PidController pid(1.0f, 0.1f, 0.01f);
float targetRpm = 100.0f;
float output = pid.compute(targetRpm, measuredRpm, dt);
```

### 既存コードについて

既存コード（MotorLogic, SerialProtocol等）はスネークケースで書かれているが、新規コードはArduinoスタイルで統一する。既存コードのリファクタリングはPhase 6で実施予定。
