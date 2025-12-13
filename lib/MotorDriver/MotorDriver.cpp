#include "MotorDriver.h"

#ifdef ARDUINO
#include <Arduino.h>
#include "HardwareConfig.h"
#endif

// =============================================================================
// コンストラクタ
// =============================================================================

MotorDriver::MotorDriver(uint8_t pinDir, uint8_t pinPwm)
    : pinDir_(pinDir)
    , pinPwm_(pinPwm)
    , currentSpeed_(0.0f)
{
}

// =============================================================================
// 初期化
// =============================================================================

void MotorDriver::begin() {
#ifdef ARDUINO
    pinMode(pinDir_, OUTPUT);
    pinMode(pinPwm_, OUTPUT);
    analogWriteFreq(HardwareConfig::PWM_FREQUENCY);
    stop();
#endif
}

// =============================================================================
// 速度設定
// =============================================================================

void MotorDriver::setSpeed(float speed) {
    currentSpeed_ = clampSpeed(speed);

#ifdef ARDUINO
    // 方向設定（LOW=正転、HIGH=逆転）
    digitalWrite(pinDir_, getDirection(currentSpeed_) ? HIGH : LOW);

    // PWM設定
    analogWrite(pinPwm_, calculatePwmDuty(currentSpeed_));
#endif
}

// =============================================================================
// 停止
// =============================================================================

void MotorDriver::stop() {
    currentSpeed_ = 0.0f;

#ifdef ARDUINO
    analogWrite(pinPwm_, 0);
#endif
}

// =============================================================================
// ブレーキ
// =============================================================================

void MotorDriver::brake() {
    // 現在の実装では通常停止と同じ
    // 将来的にブレーキ対応ドライバを使用する場合に拡張
    stop();
}

// =============================================================================
// 静的ユーティリティ関数
// =============================================================================

float MotorDriver::clampSpeed(float speed) {
    if (speed > 1.0f) {
        return 1.0f;
    }
    if (speed < -1.0f) {
        return -1.0f;
    }
    return speed;
}

bool MotorDriver::getDirection(float speed) {
    // 負の速度で逆転（DIR = HIGH = true）
    // 正の速度・0で正転（DIR = LOW = false）
    return speed < 0.0f;
}

uint8_t MotorDriver::calculatePwmDuty(float speed) {
    // 絶対値を取ってPWM値に変換
    float absSpeed = speed < 0.0f ? -speed : speed;

    // 0.0〜1.0 → 0〜255
    return static_cast<uint8_t>(absSpeed * PWM_MAX + 0.5f);
}
