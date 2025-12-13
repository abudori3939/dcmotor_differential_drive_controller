#ifndef DEBUG_LOGGER_H
#define DEBUG_LOGGER_H

#include "HardwareConfig.h"

// =============================================================================
// デバッグログ出力ユーティリティ
//
// DEBUG_BUILDが定義されている場合のみSerial1にログ出力。
// 本番ビルドでは何も出力せず、コードサイズに影響しない。
//
// 使用方法:
//   DEBUG_INIT();                    // setup()内で初期化
//   DEBUG_PRINT("メッセージ");        // 改行なし出力
//   DEBUG_PRINTLN("メッセージ");      // 改行あり出力
//   DEBUG_PRINTF("値: %d", value);   // フォーマット出力
// =============================================================================

#ifdef DEBUG_BUILD

// Arduino環境でのみSerial1を使用
#ifdef ARDUINO
#include <Arduino.h>

// デバッグシリアル初期化
#define DEBUG_INIT() \
    do { \
        Serial1.setTX(HardwareConfig::Debug::UART_TX); \
        Serial1.setRX(HardwareConfig::Debug::UART_RX); \
        Serial1.begin(HardwareConfig::Debug::UART_BAUD); \
        while (!Serial1) { delay(10); } \
        Serial1.println("=== デバッグモード開始 ==="); \
    } while(0)

// 基本出力
#define DEBUG_PRINT(msg)    Serial1.print(msg)
#define DEBUG_PRINTLN(msg)  Serial1.println(msg)
#define DEBUG_PRINTF(...)   Serial1.printf(__VA_ARGS__)

// ログレベル付き出力
#define DEBUG_INFO(msg)     Serial1.print("[INFO] "); Serial1.println(msg)
#define DEBUG_WARN(msg)     Serial1.print("[WARN] "); Serial1.println(msg)
#define DEBUG_ERROR(msg)    Serial1.print("[ERROR] "); Serial1.println(msg)

// 変数ダンプ
#define DEBUG_VAR(name, val) \
    Serial1.print(#name ": "); Serial1.println(val)

// PID制御デバッグ用
#define DEBUG_PID(target, measured, output) \
    Serial1.printf("[PID] target=%.2f measured=%.2f output=%.2f\n", \
                   (float)(target), (float)(measured), (float)(output))

// エンコーダデバッグ用
#define DEBUG_ENCODER(countL, countR, rpmL, rpmR) \
    Serial1.printf("[ENC] L=%ld R=%ld RPM: L=%.1f R=%.1f\n", \
                   (long)(countL), (long)(countR), (float)(rpmL), (float)(rpmR))

// モータ出力デバッグ用
#define DEBUG_MOTOR(dutyL, dutyR) \
    Serial1.printf("[MOT] duty: L=%.3f R=%.3f\n", (float)(dutyL), (float)(dutyR))

#else
// 非Arduino環境（ユニットテスト用）
#include <cstdio>
#define DEBUG_INIT()
#define DEBUG_PRINT(msg)    printf("%s", msg)
#define DEBUG_PRINTLN(msg)  printf("%s\n", msg)
#define DEBUG_PRINTF(...)   printf(__VA_ARGS__)
#define DEBUG_INFO(msg)     printf("[INFO] %s\n", msg)
#define DEBUG_WARN(msg)     printf("[WARN] %s\n", msg)
#define DEBUG_ERROR(msg)    printf("[ERROR] %s\n", msg)
#define DEBUG_VAR(name, val)
#define DEBUG_PID(target, measured, output)
#define DEBUG_ENCODER(countL, countR, rpmL, rpmR)
#define DEBUG_MOTOR(dutyL, dutyR)
#endif

#else
// 本番ビルド: すべて無効化（コードサイズ0）
#define DEBUG_INIT()
#define DEBUG_PRINT(msg)
#define DEBUG_PRINTLN(msg)
#define DEBUG_PRINTF(...)
#define DEBUG_INFO(msg)
#define DEBUG_WARN(msg)
#define DEBUG_ERROR(msg)
#define DEBUG_VAR(name, val)
#define DEBUG_PID(target, measured, output)
#define DEBUG_ENCODER(countL, countR, rpmL, rpmR)
#define DEBUG_MOTOR(dutyL, dutyR)
#endif

#endif  // DEBUG_LOGGER_H
