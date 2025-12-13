#ifndef SHARED_MOTOR_DATA_H
#define SHARED_MOTOR_DATA_H

#include <stdint.h>

// =============================================================================
// コア間共有データ構造体
// =============================================================================
// Core0（メインコア）とCore1（リアルタイムコア）間でデータを共有するための構造体。
// 書き込み元を固定することで競合を最小化。
//
// CmdVelData:     Core0が書き込み、Core1が読み込み
// MotorStateData: Core1が書き込み、Core0が読み込み
//
// 使用例:
//   #include "pico/mutex.h"
//
//   volatile CmdVelData cmdVelData;
//   volatile MotorStateData motorStateData;
//   mutex_t cmdVelMutex;
//   mutex_t motorStateMutex;
//
//   // 初期化（setup内で）
//   mutex_init(&cmdVelMutex);
//   mutex_init(&motorStateMutex);
//   initCmdVelData(&cmdVelData);
//   initMotorStateData(&motorStateData);
//
//   // Core0: cmd_vel書き込み
//   mutex_enter_blocking(&cmdVelMutex);
//   cmdVelData.linearX = newLinearX;
//   cmdVelData.angularZ = newAngularZ;
//   mutex_exit(&cmdVelMutex);
//
//   // Core1: cmd_vel読み込み
//   mutex_enter_blocking(&cmdVelMutex);
//   float lx = cmdVelData.linearX;
//   float az = cmdVelData.angularZ;
//   mutex_exit(&cmdVelMutex);
// =============================================================================

// =============================================================================
// CmdVelData - Core0 → Core1（コマンド入力）
// =============================================================================
// Core0が書き込み、Core1が読み込み
// ROSから受信したcmd_velとフェイルセーフ状態を共有
// =============================================================================
struct CmdVelData {
    float linearX;        // 並進速度 [m/s]
    float angularZ;       // 回転速度 [rad/s]
    bool failsafeStop;    // フェイルセーフ停止フラグ（通信途絶時にtrue）
};

// =============================================================================
// MotorStateData - Core1 → Core0（現在状態）
// =============================================================================
// Core1が書き込み、Core0が読み込み
// 制御ループで計算した現在状態をROSに送信するために共有
// =============================================================================
struct MotorStateData {
    int32_t encoderCountL;   // 左エンコーダ累積カウント
    int32_t encoderCountR;   // 右エンコーダ累積カウント
    float targetRpmL;        // 目標RPM（左）- cmd_velから計算
    float targetRpmR;        // 目標RPM（右）- cmd_velから計算
    float currentRpmL;       // 現在RPM（左）- エンコーダから計算
    float currentRpmR;       // 現在RPM（右）- エンコーダから計算
};

// =============================================================================
// 初期化関数
// =============================================================================

/**
 * CmdVelDataを初期値でクリア
 * @param data 初期化する構造体へのポインタ
 */
inline void initCmdVelData(volatile CmdVelData* data) {
    data->linearX = 0.0f;
    data->angularZ = 0.0f;
    data->failsafeStop = false;
}

/**
 * MotorStateDataを初期値でクリア
 * @param data 初期化する構造体へのポインタ
 */
inline void initMotorStateData(volatile MotorStateData* data) {
    data->encoderCountL = 0;
    data->encoderCountR = 0;
    data->targetRpmL = 0.0f;
    data->targetRpmR = 0.0f;
    data->currentRpmL = 0.0f;
    data->currentRpmR = 0.0f;
}

#endif  // SHARED_MOTOR_DATA_H
