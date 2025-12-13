#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include <stdint.h>

namespace HardwareConfig {

// =============================================================================
// エンコーダピン（2相エンコーダ A/B相）
// =============================================================================
constexpr uint8_t ENCODER_L_A = 2;
constexpr uint8_t ENCODER_L_B = 3;
constexpr uint8_t ENCODER_R_A = 4;
constexpr uint8_t ENCODER_R_B = 5;

// =============================================================================
// モータドライバピン（方向 + PWM）
// =============================================================================
constexpr uint8_t MOTOR_L_DIR = 6;
constexpr uint8_t MOTOR_L_PWM = 7;
constexpr uint8_t MOTOR_R_DIR = 8;
constexpr uint8_t MOTOR_R_PWM = 9;

// =============================================================================
// PWM設定
// =============================================================================
constexpr uint32_t PWM_FREQUENCY = 20000;  // 20kHz（可聴域外）

// =============================================================================
// 制御ループタイミング
// =============================================================================
constexpr uint32_t CONTROL_PERIOD_US = 10000;  // 10ms（100Hz）

// =============================================================================
// フェイルセーフ設定
// =============================================================================
constexpr uint32_t FAILSAFE_TIMEOUT_MS = 500;  // 通信途絶時にモータ停止

// =============================================================================
// デフォルト設定値
// =============================================================================
namespace Defaults {
    constexpr float PID_KP = 1.0f;
    constexpr float PID_KI = 0.1f;
    constexpr float PID_KD = 0.01f;
    constexpr float MAX_RPM = 200.0f;
    constexpr uint16_t ENCODER_PPR = 1024;
    constexpr float GEAR_RATIO = 1.0f;
}

// =============================================================================
// デバッグ用UART（Serial1経由、Debug Probeで受信）
// =============================================================================
namespace Debug {
    constexpr uint8_t UART_TX = 0;  // GPIO0
    constexpr uint8_t UART_RX = 1;  // GPIO1
    constexpr uint32_t UART_BAUD = 115200;
}

// =============================================================================
// ファームウェアバージョン
// =============================================================================
namespace Version {
    constexpr uint8_t MAJOR = 0;
    constexpr uint8_t MINOR = 1;
    constexpr uint8_t PATCH = 0;
}

}  // namespace HardwareConfig

#endif  // HARDWARE_CONFIG_H
