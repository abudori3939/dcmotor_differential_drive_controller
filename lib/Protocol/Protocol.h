/**
 * @file Protocol.h
 * @brief ROS-Pico間通信プロトコル
 *
 * リクエスト・レスポンス型プロトコルのパケット作成・パース処理。
 * 詳細仕様は documents/protocol.md を参照。
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

namespace Protocol {

// =============================================================================
// 定数定義
// =============================================================================

// リクエストタイプ
constexpr uint8_t REQUEST_MOTOR_COMMAND = 0x00;
constexpr uint8_t REQUEST_GET_VERSION = 0x01;
constexpr uint8_t REQUEST_GET_STATUS = 0x02;
constexpr uint8_t REQUEST_GET_CONFIG = 0x03;
constexpr uint8_t REQUEST_SET_CONFIG = 0x04;
constexpr uint8_t REQUEST_GET_DEBUG_OUTPUT = 0x05;

// ヘッダオフセット
constexpr uint8_t HEADER_REQUEST_TYPE = 0;
constexpr uint8_t HEADER_PAYLOAD_LENGTH = 1;
constexpr uint8_t HEADER_CHECKSUM_L = 2;
constexpr uint8_t HEADER_CHECKSUM_H = 3;
constexpr uint8_t HEADER_SIZE = 4;

// ステータスフラグ
constexpr uint16_t STATUS_FAILSAFE = (1 << 0);
constexpr uint16_t STATUS_ENCODER_L_ERROR = (1 << 1);
constexpr uint16_t STATUS_ENCODER_R_ERROR = (1 << 2);
constexpr uint16_t STATUS_MOTOR_L_ERROR = (1 << 3);
constexpr uint16_t STATUS_MOTOR_R_ERROR = (1 << 4);
constexpr uint16_t STATUS_CONFIG_EMPTY = (1 << 5);
constexpr uint16_t STATUS_FLASH_ERROR = (1 << 6);
constexpr uint16_t STATUS_OVERTEMP = (1 << 7);
constexpr uint16_t STATUS_OVERCURRENT = (1 << 8);
constexpr uint16_t STATUS_LOW_VOLTAGE = (1 << 9);
constexpr uint16_t STATUS_CONFIG_MODE = (1 << 15);

// エラーコード
constexpr uint8_t ERROR_NONE = 0x00;
constexpr uint8_t ERROR_CHECKSUM = 0x01;
constexpr uint8_t ERROR_INVALID_COMMAND = 0x02;
constexpr uint8_t ERROR_PAYLOAD = 0x03;
constexpr uint8_t ERROR_ENCODER_TIMEOUT = 0x10;
constexpr uint8_t ERROR_FLASH = 0x20;

// SET_CONFIG結果
constexpr uint8_t CONFIG_RESULT_SUCCESS = 0x00;
constexpr uint8_t CONFIG_RESULT_FLASH_ERROR = 0x01;
constexpr uint8_t CONFIG_RESULT_INVALID_VALUE = 0x02;

// =============================================================================
// データ構造体
// =============================================================================

// MOTOR_COMMANDリクエストのペイロード
struct MotorCommandRequest {
    float linearX;
    float angularZ;
};

// MOTOR_COMMANDレスポンスのペイロード
struct MotorCommandResponse {
    int32_t encoderCountL;
    int32_t encoderCountR;
    uint16_t status;
};

// GET_VERSIONレスポンスのペイロード
struct VersionResponse {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
};

// GET_STATUSレスポンスのペイロード
struct StatusResponse {
    uint16_t status;
    uint8_t errorCode;
    uint16_t commErrorCount;
    uint32_t uptimeMs;
};

// GET_CONFIG / SET_CONFIG共通データ
struct ConfigData {
    float pidKp;
    float pidKi;
    float pidKd;
    float maxRpm;
    uint16_t encoderPpr;
    float gearRatio;
    float wheelDiameter;
    float trackWidth;
};

// GET_DEBUG_OUTPUTレスポンスのペイロード
struct DebugOutputResponse {
    int32_t encoderCountL;
    int32_t encoderCountR;
    float targetRpmL;
    float targetRpmR;
    float currentRpmL;
    float currentRpmR;
    float pwmDutyL;
    float pwmDutyR;
};

// =============================================================================
// パース結果
// =============================================================================

enum ParseResult {
    PARSE_OK = 0,
    PARSE_ERROR_SIZE,
    PARSE_ERROR_CHECKSUM,
    PARSE_ERROR_INVALID_TYPE
};

// パースされたリクエスト
struct ParsedRequest {
    uint8_t requestType;
    uint8_t payloadLength;

    // リクエストタイプに応じたデータ
    union {
        MotorCommandRequest motorCommand;
        ConfigData setConfig;
    };
};

// =============================================================================
// 関数宣言
// =============================================================================

/**
 * チェックサム計算（16bit加算）
 * @param data ペイロードデータ
 * @param length データ長
 * @return チェックサム値
 */
uint16_t calculateChecksum(const uint8_t* data, size_t length);

/**
 * リクエストパケットをパース
 * @param packet パケットデータ
 * @param packetLength パケット長
 * @param result パース結果を格納する構造体
 * @return パース結果
 */
ParseResult parseRequest(const uint8_t* packet, size_t packetLength, ParsedRequest& result);

/**
 * MOTOR_COMMANDレスポンス作成
 * @param data レスポンスデータ
 * @param buffer 出力バッファ
 * @param bufferSize バッファサイズ
 * @return 作成されたパケット長
 */
uint8_t createMotorCommandResponse(const MotorCommandResponse& data, uint8_t* buffer, size_t bufferSize);

/**
 * GET_VERSIONレスポンス作成
 */
uint8_t createVersionResponse(const VersionResponse& data, uint8_t* buffer, size_t bufferSize);

/**
 * GET_STATUSレスポンス作成
 */
uint8_t createStatusResponse(const StatusResponse& data, uint8_t* buffer, size_t bufferSize);

/**
 * GET_CONFIGレスポンス作成
 */
uint8_t createConfigResponse(const ConfigData& data, uint8_t* buffer, size_t bufferSize);

/**
 * GET_DEBUG_OUTPUTレスポンス作成
 */
uint8_t createDebugOutputResponse(const DebugOutputResponse& data, uint8_t* buffer, size_t bufferSize);

/**
 * SET_CONFIGレスポンス作成
 * @param result 結果コード（CONFIG_RESULT_*）
 */
uint8_t createSetConfigResponse(uint8_t result, uint8_t* buffer, size_t bufferSize);

}  // namespace Protocol

#endif  // PROTOCOL_H
