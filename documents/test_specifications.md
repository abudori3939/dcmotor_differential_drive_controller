# テスト仕様書

TDD開発用のテストケース仕様。各ライブラリの期待動作を定義。

## テスト方針

- ハードウェア非依存のロジックはすべてユニットテスト対象
- テストファースト: テストを先に書き、実装は後
- テストファイルは `test/test_<ライブラリ名>/` に配置

## PIDController テスト仕様

### 基本動作

| テストケース | 入力 | 期待出力 |
|-------------|------|---------|
| P制御のみ（Kp=1.0, Ki=0, Kd=0） | setpoint=100, measured=0 | 出力=100（誤差×Kp） |
| 誤差ゼロ | setpoint=50, measured=50 | 出力=0 |
| 負の誤差 | setpoint=0, measured=100 | 出力=-100 |

### 積分項（I制御）

| テストケース | 条件 | 期待動作 |
|-------------|------|---------|
| 積分蓄積 | 同じ誤差を複数回compute | 出力が徐々に増加 |
| reset()後 | reset()呼び出し後 | 積分値がクリア |

### 微分項（D制御）

| テストケース | 条件 | 期待動作 |
|-------------|------|---------|
| 誤差変化なし | 2回連続同じ誤差 | D項=0 |
| 誤差増加 | 誤差が増加 | D項が正の追加出力 |
| 誤差減少 | 誤差が減少 | D項が負の追加出力 |

### 出力リミット

| テストケース | 条件 | 期待動作 |
|-------------|------|---------|
| 上限クランプ | setOutputLimits(-100, 100)、出力>100 | 100にクランプ |
| 下限クランプ | 出力<-100 | -100にクランプ |
| アンチワインドアップ | 出力飽和中の積分 | 積分値が過大にならない |

### テストコード例

```cpp
void test_pid_p_control_only(void) {
    PIDController pid(1.0f, 0.0f, 0.0f);  // Kp=1, Ki=0, Kd=0
    float output = pid.compute(100.0f, 0.0f, 0.01f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, output);
}

void test_pid_zero_error(void) {
    PIDController pid(1.0f, 0.0f, 0.0f);
    float output = pid.compute(50.0f, 50.0f, 0.01f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, output);
}

void test_pid_output_limits(void) {
    PIDController pid(10.0f, 0.0f, 0.0f);
    pid.setOutputLimits(-100.0f, 100.0f);
    float output = pid.compute(100.0f, 0.0f, 0.01f);  // 誤差100 × Kp10 = 1000
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, output);  // 100にクランプ
}
```

## QuadratureEncoder テスト仕様

ハードウェア依存部分を除いたロジックのテスト。

### RPM計算

| テストケース | 入力 | 期待出力 |
|-------------|------|---------|
| 正転 | カウント差+1024、dt=1.0s、PPR=1024 | RPM=60 |
| 逆転 | カウント差-1024、dt=1.0s、PPR=1024 | RPM=-60 |
| 停止 | カウント差0 | RPM=0 |
| 高速回転 | カウント差+2048、dt=0.5s、PPR=1024 | RPM=240 |

### 計算式

```
RPM = (カウント差 / PPR) / dt × 60
```

### テストコード例

```cpp
void test_encoder_rpm_calculation(void) {
    // PPR=1024、1秒間に1024カウント = 1回転/秒 = 60RPM
    float rpm = calculate_rpm(1024, 1024, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 60.0f, rpm);
}

void test_encoder_rpm_reverse(void) {
    float rpm = calculate_rpm(-1024, 1024, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, -60.0f, rpm);
}
```

## DifferentialKinematics テスト仕様

cmd_velから左右ホイールRPMへの変換テスト。

### 計算式

```
wheel_radius = wheel_diameter / 2
left_vel  = linear_x - angular_z * track_width / 2  [m/s]
right_vel = linear_x + angular_z * track_width / 2  [m/s]
left_rpm  = left_vel / (2 * PI * wheel_radius) * 60 * gear_ratio
right_rpm = right_vel / (2 * PI * wheel_radius) * 60 * gear_ratio
```

### テストケース

テスト条件: wheel_diameter=0.1m, track_width=0.3m, gear_ratio=1.0

| テストケース | linear_x | angular_z | 期待left_rpm | 期待right_rpm |
|-------------|----------|-----------|--------------|---------------|
| 前進のみ | 0.1 m/s | 0 rad/s | 19.1 | 19.1 |
| 後退のみ | -0.1 m/s | 0 rad/s | -19.1 | -19.1 |
| 左旋回（その場） | 0 m/s | 1.0 rad/s | -28.6 | 28.6 |
| 右旋回（その場） | 0 m/s | -1.0 rad/s | 28.6 | -28.6 |
| 前進+左旋回 | 0.1 m/s | 0.5 rad/s | 4.8 | 33.4 |
| 停止 | 0 m/s | 0 rad/s | 0 | 0 |

### テストコード例

```cpp
void test_kinematics_forward(void) {
    // wheel_diameter=0.1m, track_width=0.3m, gear_ratio=1.0
    float left_rpm, right_rpm;
    calculate_wheel_rpm(0.1f, 0.0f, 0.1f, 0.3f, 1.0f, left_rpm, right_rpm);
    // 0.1 / (2 * PI * 0.05) * 60 = 19.0986...
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 19.1f, left_rpm);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 19.1f, right_rpm);
}

void test_kinematics_rotate_left(void) {
    float left_rpm, right_rpm;
    calculate_wheel_rpm(0.0f, 1.0f, 0.1f, 0.3f, 1.0f, left_rpm, right_rpm);
    // left_vel = 0 - 1.0 * 0.15 = -0.15 m/s → -28.6 RPM
    // right_vel = 0 + 1.0 * 0.15 = 0.15 m/s → 28.6 RPM
    TEST_ASSERT_FLOAT_WITHIN(0.1f, -28.6f, left_rpm);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 28.6f, right_rpm);
}
```

## ConfigStorage テスト仕様

Flashアクセスはモック化してテスト。

### 設定読み書き

| テストケース | 条件 | 期待動作 |
|-------------|------|---------|
| 保存→読み込み | save()後にload() | 同じ値が返る |
| チェックサム不正 | 破損データ | load()がfalse、デフォルト値使用 |
| 初回起動 | Flash未初期化 | load()がfalse、デフォルト値使用 |
| リセット | resetToDefaults() | デフォルト値に戻る |

### デフォルト値

```cpp
const RobotConfig DEFAULT_CONFIG = {
    .pid_kp = 1.0f,
    .pid_ki = 0.1f,
    .pid_kd = 0.01f,
    .max_rpm = 200.0f,
    .encoder_ppr = 1024,
    .gear_ratio = 1.0f,
    .wheel_diameter = 0.1f,   // 100mm
    .track_width = 0.3f       // 300mm
};
```

## 既存ライブラリテスト（参考）

### MotorLogic

既存テスト: `test/test_motor_logic/test_motor_logic.cpp`

| テストケース | 内容 |
|-------------|------|
| clamp_rpm_simple | 上限超過時のクランプ |
| clamp_rpm_rotation_priority | 回転成分優先クランプ |
| check_max_rpm | 製品ID別の最大RPM |

### SerialProtocol

既存テスト: `test/test_serial_protocol/test_serial_protocol.cpp`

| テストケース | 内容 |
|-------------|------|
| calculate_checksum | チェックサム計算 |
| write/read_*_to_buf | バッファ操作 |

## テスト実行

```bash
# 全テスト実行
pio test -e native

# 特定ライブラリのみ
pio test -e native -f test_pid_controller
pio test -e native -f test_motor_logic
```
