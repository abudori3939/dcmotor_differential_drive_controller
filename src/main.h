/**
 * @file main.h
 * @brief メインアプリケーション用ヘッダ
 *
 * グローバル変数のextern宣言、設定構造体の定義を配置。
 */

#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include "SharedMotorData.h"
#include "HardwareConfig.h"

// =============================================================================
// 設定構造体
// =============================================================================

/**
 * ロボット設定（将来ConfigStorageでFlash保存）
 */
struct RobotConfig {
    float pidKp;
    float pidKi;
    float pidKd;
    float maxRpm;
    uint16_t encoderPpr;
    float gearRatio;
    float wheelDiameter;
    float trackWidth;

    // デフォルト値で初期化
    RobotConfig() :
        pidKp(HardwareConfig::Defaults::PID_KP),
        pidKi(HardwareConfig::Defaults::PID_KI),
        pidKd(HardwareConfig::Defaults::PID_KD),
        maxRpm(HardwareConfig::Defaults::MAX_RPM),
        encoderPpr(HardwareConfig::Defaults::ENCODER_PPR),
        gearRatio(HardwareConfig::Defaults::GEAR_RATIO),
        wheelDiameter(0.1f),   // 100mm
        trackWidth(0.3f)       // 300mm
    {}
};

// =============================================================================
// ステータス管理
// =============================================================================

/**
 * システムステータス
 */
struct SystemStatus {
    uint16_t flags;           // Protocol::STATUS_* フラグ
    uint8_t lastErrorCode;    // Protocol::ERROR_* コード
    uint16_t commErrorCount;  // 通信エラー累積

    SystemStatus() : flags(0), lastErrorCode(0), commErrorCount(0) {}
};

// =============================================================================
// extern宣言（main.cppで定義）
// =============================================================================

// 共有データ（コア間通信）
extern volatile CmdVelData cmdVelData;
extern volatile MotorStateData motorStateData;

// 設定・ステータス
extern RobotConfig config;
extern SystemStatus systemStatus;

// フェイルセーフ
extern unsigned long lastCommandTimeMs;

#endif  // MAIN_H
