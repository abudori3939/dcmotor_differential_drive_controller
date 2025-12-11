# アーキテクチャ設計

DCモータ差動二輪ロボット汎用コントローラの設計ドキュメント。

## 概要

このプロジェクトはCuGo専用ファームウェアから汎用DCモータ差動二輪ロボット向けに拡張中。

### 現状（CuGo専用）→ 新仕様（汎用）の変更点

| 機能 | 現状（CuGo） | 新仕様（汎用） |
|------|-------------|---------------|
| エンコーダ取得 | LD-2からSerial1受信 | **Picoピン入力で2相エンコーダ直読** |
| 速度制御 | LD-2任せ | **Pico内でPID制御** |
| モータ出力 | RPMをSerial1送信 | **PWM + 方向ピン(H/L)** |
| ROS通信 | PacketSerial | 変更なし |
| RC/CMDモード | LD-2依存 | 削除（将来別途実装） |
| タイマー | main.cppの自作実装 | **ハードウェアタイマー割り込み** |
| 設定変更 | コード書き換え | **設定ツールでFlash書き込み** |

## デュアルコア構成

Raspberry Pi Picoのデュアルコアを活用してリアルタイム性を確保。

### Core0（メインコア）
- ROS通信（PacketSerial）
- コマンド解析・目標RPM受信
- フェイルセーフ監視
- 設定モード処理
- エンコーダカウントをROSに送信

### Core1（リアルタイムコア）
- エンコーダ割り込み処理
- PID制御ループ（高周期実行）
- モータPWM出力

### コア間通信（Shared Memory + Mutex）

共有メモリとMutexによるデータ共有。最新値を常に参照可能。

```cpp
#include "pico/mutex.h"

// 共有データ構造
struct SharedMotorData {
    // Core0 → Core1（目標値）
    float target_rpm_l;
    float target_rpm_r;
    bool failsafe_stop;

    // Core1 → Core0（現在値）
    long encoder_count_l;
    long encoder_count_r;
    float current_rpm_l;
    float current_rpm_r;
};

volatile SharedMotorData shared_data;
mutex_t motor_mutex;

// Core0での書き込み
mutex_enter_blocking(&motor_mutex);
shared_data.target_rpm_l = new_target_l;
shared_data.target_rpm_r = new_target_r;
mutex_exit(&motor_mutex);

// Core1での読み込み
mutex_enter_blocking(&motor_mutex);
float local_target_l = shared_data.target_rpm_l;
float local_target_r = shared_data.target_rpm_r;
mutex_exit(&motor_mutex);
```

## 新アーキテクチャ

```
┌─────────────────────────────────────────────────────────────┐
│                    main.cpp (Core0)                         │
│  ・ROS通信（PacketSerial）                                   │
│  ・コマンド解析                                              │
│  ・フェイルセーフ                                            │
│  ・設定モード処理                                            │
└─────────────────────────────────────────────────────────────┘
              │ Shared Memory + Mutex
              ▼
┌─────────────────────────────────────────────────────────────┐
│                MotorController (Core1)                       │
│  ・左右モータの統合制御                                       │
│  ・目標RPM → PID → PWM出力の制御ループ                        │
└─────────────────────────────────────────────────────────────┘
              │                               │
              ▼                               ▼
┌─────────────────────────┐     ┌─────────────────────────┐
│   QuadratureEncoder     │     │      MotorDriver        │
│  ・2相エンコーダ読み取り  │     │  ・PWM出力              │
│  ・カウント累積          │     │  ・方向ピン(H/L)         │
│  ・RPM計算              │     │                         │
└─────────────────────────┘     └─────────────────────────┘
              │
              ▼
┌─────────────────────────┐
│     PIDController       │
│  ・PID計算              │
│  ・ゲイン設定           │
└─────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│  既存ライブラリ（変更なし）                                   │
│  ・SerialProtocol: パケット処理、チェックサム                 │
│  ・MotorLogic: RPMクランプ                                   │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                   ConfigStorage (新規)                       │
│  ・Flash読み書き（EEPROM/LittleFS）                          │
│  ・PIDゲイン、最高速度の永続化                                │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                   HardwareConfig (新規)                      │
│  ・ピンアサイン定義                                          │
│  ・エンコーダPPR設定                                         │
│  ・PWM周波数設定                                             │
└─────────────────────────────────────────────────────────────┘
```

## ライブラリ一覧

### 既存ライブラリ（変更なし）

| ライブラリ | 役割 | テスト |
|-----------|------|-------|
| SerialProtocol | ROS通信プロトコル | ○ |
| MotorLogic | RPMクランプ | ○ |

### 新規ライブラリ

| ライブラリ | 役割 | テスト | 実行コア |
|-----------|------|-------|---------|
| PIDController | PID制御演算 | ○ | Core1 |
| QuadratureEncoder | 2相エンコーダ読み取り | △（ロジック部のみ） | Core1 |
| MotorDriver | PWM+方向出力 | × | Core1 |
| MotorController | モータ制御統合 | △（ロジック部のみ） | Core1 |
| ConfigStorage | Flash設定保存 | × | Core0 |
| HardwareConfig | ピン・パラメータ設定 | × | 両方 |

### 削除予定

| ライブラリ | 理由 |
|-----------|------|
| CugoSDK | LD-2専用のため汎用版では不要 |

## 設定保存・変更方式

### 設定項目

| 項目 | 型 | デフォルト値 |
|------|---|-------------|
| PID Kp | float | 1.0 |
| PID Ki | float | 0.1 |
| PID Kd | float | 0.01 |
| 最高速度 (RPM) | float | 200.0 |
| エンコーダPPR | uint16_t | 1024 |

### 設定変更方式

ROS側から `SET_CONFIG` リクエスト（0x04）を送信することで設定をFlashに保存する。
専用の設定ツールは不要で、ROS 2パッケージから直接設定変更が可能。

#### 動作フロー

1. ROS側から `SET_CONFIG` リクエスト送信
2. Picoが設定値を受信・検証
3. `EEPROM`または`LittleFS`でFlashに永続化
4. 結果をレスポンスで返信（成功/失敗）

詳細は `documents/protocol.md` の `0x04: SET_CONFIG` を参照。

## 新規ライブラリ詳細設計

### PIDController

PID制御の演算ロジック。ハードウェア非依存でユニットテスト可能。

```cpp
class PIDController {
public:
    PIDController(float kp, float ki, float kd);

    void setGains(float kp, float ki, float kd);
    void setOutputLimits(float min, float max);
    void reset();

    float compute(float setpoint, float measured, float dt);

private:
    float kp_, ki_, kd_;
    float integral_;
    float prev_error_;
    float output_min_, output_max_;
};
```

### QuadratureEncoder

2相エンコーダ（A/B相）からパルスカウントとRPMを取得。

```cpp
class QuadratureEncoder {
public:
    QuadratureEncoder(uint8_t pin_a, uint8_t pin_b, uint16_t ppr);

    void begin();                    // 割り込み設定
    long getCount();                 // 累積カウント取得
    void resetCount();               // カウントリセット
    float getRPM(float dt);          // RPM計算

private:
    uint8_t pin_a_, pin_b_;
    uint16_t ppr_;                   // Pulses Per Revolution
    volatile long count_;
    long prev_count_;

    static void handleInterrupt();   // 割り込みハンドラ
};
```

### MotorDriver

汎用DCモータドライバ（方向+PWM方式）への出力。

```cpp
class MotorDriver {
public:
    MotorDriver(uint8_t pin_dir, uint8_t pin_pwm);

    void begin();
    void setSpeed(float speed);      // -1.0 ~ 1.0（負で逆転）
    void stop();
    void brake();                    // 急停止（ドライバ対応時）

private:
    uint8_t pin_dir_, pin_pwm_;
};
```

### MotorController

左右モータの統合制御。PID制御ループを内包。Core1で実行。

```cpp
class MotorController {
public:
    MotorController(
        QuadratureEncoder& encoder_l, QuadratureEncoder& encoder_r,
        MotorDriver& driver_l, MotorDriver& driver_r,
        PIDController& pid_l, PIDController& pid_r
    );

    void begin();
    void setTargetRPM(float left, float right);
    void update(float dt);           // 制御ループ（定期呼び出し）

    float getCurrentRPM_L();
    float getCurrentRPM_R();
    long getEncoderCount_L();
    long getEncoderCount_R();

private:
    // 各コンポーネントへの参照
};
```

### ConfigStorage

Flashへの設定保存。

```cpp
struct RobotConfig {
    float pid_kp;
    float pid_ki;
    float pid_kd;
    float max_rpm;
    uint16_t encoder_ppr;
    uint8_t checksum;  // 設定有効性確認用
};

class ConfigStorage {
public:
    void begin();
    bool load(RobotConfig& config);
    void save(const RobotConfig& config);
    void resetToDefaults();

private:
    static const RobotConfig DEFAULT_CONFIG;
};
```

### HardwareConfig

ハードウェア設定の一元管理。

```cpp
// HardwareConfig.h
namespace HardwareConfig {
    // エンコーダピン
    constexpr uint8_t ENCODER_L_A = 2;
    constexpr uint8_t ENCODER_L_B = 3;
    constexpr uint8_t ENCODER_R_A = 4;
    constexpr uint8_t ENCODER_R_B = 5;

    // モータドライバピン
    constexpr uint8_t MOTOR_L_DIR = 6;
    constexpr uint8_t MOTOR_L_PWM = 7;
    constexpr uint8_t MOTOR_R_DIR = 8;
    constexpr uint8_t MOTOR_R_PWM = 9;

    // PWM設定
    constexpr uint32_t PWM_FREQUENCY = 20000;  // 20kHz

    // 制御周期
    constexpr uint32_t CONTROL_PERIOD_US = 10000;  // 10ms (100Hz)
}
```

## 制御ループ

### Core1（リアルタイム処理）

```
10ms周期（100Hz）ハードウェアタイマー割り込み:
  1. Mutex取得 → 共有メモリから目標RPM読み込み
  2. エンコーダからカウント取得
  3. RPM計算
  4. PID制御で出力値計算
  5. モータドライバへPWM出力
  6. Mutex取得 → 共有メモリにエンコーダカウント・現在RPM書き込み
```

### Core0（通信処理）

```
loop():
  1. PacketSerial.update() - ROS通信処理
  2. Mutex取得 → 共有メモリからエンコーダカウント読み込み
  3. フェイルセーフチェック（500ms通信途絶でfailsafe_stop=true）
  4. 設定モード処理（設定コマンド受信時）
```

## 実装順序

1. **HardwareConfig** - ピン定義
2. **PIDController** - ハードウェア非依存、テスト作成
3. **QuadratureEncoder** - エンコーダ読み取り
4. **MotorDriver** - PWM出力
5. **MotorController** - 統合（Core1実行）
6. **ConfigStorage** - Flash設定保存
7. **main.cpp改修** - デュアルコア構成、CugoSDK削除

## Raspberry Pi Pico固有API（PlatformIO/Arduino）

使用するarduino-picoコアのAPI:

| 機能 | API |
|------|-----|
| デュアルコア | `setup1()` / `loop1()` |
| Mutex | `#include "pico/mutex.h"` / `mutex_init()` / `mutex_enter_blocking()` / `mutex_exit()` |
| タイマー割り込み | `add_repeating_timer_us()` (pico-sdk) |
| GPIO割り込み | `attachInterrupt()` |
| PWM | `analogWriteFreq()` + `analogWrite()` |
| Flash保存 | `EEPROM` または `LittleFS` |
