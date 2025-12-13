/**
 * @file Protocol.cpp
 * @brief ROS-Pico間通信プロトコル実装
 */

#include "Protocol.h"
#include <cstring>

namespace Protocol {

// =============================================================================
// チェックサム計算
// =============================================================================

uint16_t calculateChecksum(const uint8_t* data, size_t length) {
    uint16_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
}

// =============================================================================
// ヘルパー関数
// =============================================================================

namespace {

/**
 * ヘッダを作成してバッファに書き込み
 */
void writeHeader(uint8_t* buffer, uint8_t responseType, uint8_t payloadLength, uint16_t checksum) {
    buffer[HEADER_REQUEST_TYPE] = responseType;
    buffer[HEADER_PAYLOAD_LENGTH] = payloadLength;
    buffer[HEADER_CHECKSUM_L] = checksum & 0xFF;
    buffer[HEADER_CHECKSUM_H] = (checksum >> 8) & 0xFF;
}

/**
 * リクエストタイプが有効か確認
 */
bool isValidRequestType(uint8_t requestType) {
    switch (requestType) {
        case REQUEST_MOTOR_COMMAND:
        case REQUEST_GET_VERSION:
        case REQUEST_GET_STATUS:
        case REQUEST_GET_CONFIG:
        case REQUEST_SET_CONFIG:
        case REQUEST_GET_DEBUG_OUTPUT:
            return true;
        default:
            return false;
    }
}

}  // namespace

// =============================================================================
// リクエストパース
// =============================================================================

ParseResult parseRequest(const uint8_t* packet, size_t packetLength, ParsedRequest& result) {
    // 最小サイズチェック（ヘッダ4バイト）
    if (packetLength < HEADER_SIZE) {
        return PARSE_ERROR_SIZE;
    }

    uint8_t requestType = packet[HEADER_REQUEST_TYPE];
    uint8_t payloadLength = packet[HEADER_PAYLOAD_LENGTH];
    uint16_t receivedChecksum = packet[HEADER_CHECKSUM_L] | (packet[HEADER_CHECKSUM_H] << 8);

    // リクエストタイプ検証
    if (!isValidRequestType(requestType)) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    // パケットサイズ検証
    if (packetLength < HEADER_SIZE + payloadLength) {
        return PARSE_ERROR_SIZE;
    }

    // チェックサム検証
    const uint8_t* payload = packet + HEADER_SIZE;
    uint16_t calculatedChecksum = calculateChecksum(payload, payloadLength);
    if (receivedChecksum != calculatedChecksum) {
        return PARSE_ERROR_CHECKSUM;
    }

    // 結果を格納
    result.requestType = requestType;
    result.payloadLength = payloadLength;

    // リクエストタイプに応じてペイロードをパース
    switch (requestType) {
        case REQUEST_MOTOR_COMMAND:
            if (payloadLength >= 8) {
                memcpy(&result.motorCommand.linearX, payload, 4);
                memcpy(&result.motorCommand.angularZ, payload + 4, 4);
            }
            break;

        case REQUEST_SET_CONFIG:
            if (payloadLength >= 30) {
                memcpy(&result.setConfig.pidKp, payload, 4);
                memcpy(&result.setConfig.pidKi, payload + 4, 4);
                memcpy(&result.setConfig.pidKd, payload + 8, 4);
                memcpy(&result.setConfig.maxRpm, payload + 12, 4);
                memcpy(&result.setConfig.encoderPpr, payload + 16, 2);
                memcpy(&result.setConfig.gearRatio, payload + 18, 4);
                memcpy(&result.setConfig.wheelDiameter, payload + 22, 4);
                memcpy(&result.setConfig.trackWidth, payload + 26, 4);
            }
            break;

        default:
            // ペイロードなしのリクエストは何もしない
            break;
    }

    return PARSE_OK;
}

// =============================================================================
// レスポンス作成
// =============================================================================

uint8_t createMotorCommandResponse(const MotorCommandResponse& data, uint8_t* buffer, size_t bufferSize) {
    constexpr uint8_t PAYLOAD_LENGTH = 10;
    constexpr uint8_t PACKET_LENGTH = HEADER_SIZE + PAYLOAD_LENGTH;

    if (bufferSize < PACKET_LENGTH) {
        return 0;
    }

    // ペイロード作成
    uint8_t* payload = buffer + HEADER_SIZE;
    memcpy(payload, &data.encoderCountL, 4);
    memcpy(payload + 4, &data.encoderCountR, 4);
    memcpy(payload + 8, &data.status, 2);

    // ヘッダ作成
    uint16_t checksum = calculateChecksum(payload, PAYLOAD_LENGTH);
    writeHeader(buffer, REQUEST_MOTOR_COMMAND, PAYLOAD_LENGTH, checksum);

    return PACKET_LENGTH;
}

uint8_t createVersionResponse(const VersionResponse& data, uint8_t* buffer, size_t bufferSize) {
    constexpr uint8_t PAYLOAD_LENGTH = 4;
    constexpr uint8_t PACKET_LENGTH = HEADER_SIZE + PAYLOAD_LENGTH;

    if (bufferSize < PACKET_LENGTH) {
        return 0;
    }

    // ペイロード作成
    uint8_t* payload = buffer + HEADER_SIZE;
    payload[0] = data.major;
    payload[1] = data.minor;
    payload[2] = data.patch;
    payload[3] = 0;  // reserved

    // ヘッダ作成
    uint16_t checksum = calculateChecksum(payload, PAYLOAD_LENGTH);
    writeHeader(buffer, REQUEST_GET_VERSION, PAYLOAD_LENGTH, checksum);

    return PACKET_LENGTH;
}

uint8_t createStatusResponse(const StatusResponse& data, uint8_t* buffer, size_t bufferSize) {
    constexpr uint8_t PAYLOAD_LENGTH = 12;
    constexpr uint8_t PACKET_LENGTH = HEADER_SIZE + PAYLOAD_LENGTH;

    if (bufferSize < PACKET_LENGTH) {
        return 0;
    }

    // ペイロード作成
    uint8_t* payload = buffer + HEADER_SIZE;
    memcpy(payload, &data.status, 2);
    payload[2] = data.errorCode;
    payload[3] = 0;  // reserved
    memcpy(payload + 4, &data.commErrorCount, 2);
    payload[6] = 0;  // reserved
    payload[7] = 0;  // reserved
    memcpy(payload + 8, &data.uptimeMs, 4);

    // ヘッダ作成
    uint16_t checksum = calculateChecksum(payload, PAYLOAD_LENGTH);
    writeHeader(buffer, REQUEST_GET_STATUS, PAYLOAD_LENGTH, checksum);

    return PACKET_LENGTH;
}

uint8_t createConfigResponse(const ConfigData& data, uint8_t* buffer, size_t bufferSize) {
    constexpr uint8_t PAYLOAD_LENGTH = 30;
    constexpr uint8_t PACKET_LENGTH = HEADER_SIZE + PAYLOAD_LENGTH;

    if (bufferSize < PACKET_LENGTH) {
        return 0;
    }

    // ペイロード作成
    uint8_t* payload = buffer + HEADER_SIZE;
    memcpy(payload, &data.pidKp, 4);
    memcpy(payload + 4, &data.pidKi, 4);
    memcpy(payload + 8, &data.pidKd, 4);
    memcpy(payload + 12, &data.maxRpm, 4);
    memcpy(payload + 16, &data.encoderPpr, 2);
    memcpy(payload + 18, &data.gearRatio, 4);
    memcpy(payload + 22, &data.wheelDiameter, 4);
    memcpy(payload + 26, &data.trackWidth, 4);

    // ヘッダ作成
    uint16_t checksum = calculateChecksum(payload, PAYLOAD_LENGTH);
    writeHeader(buffer, REQUEST_GET_CONFIG, PAYLOAD_LENGTH, checksum);

    return PACKET_LENGTH;
}

uint8_t createDebugOutputResponse(const DebugOutputResponse& data, uint8_t* buffer, size_t bufferSize) {
    constexpr uint8_t PAYLOAD_LENGTH = 32;
    constexpr uint8_t PACKET_LENGTH = HEADER_SIZE + PAYLOAD_LENGTH;

    if (bufferSize < PACKET_LENGTH) {
        return 0;
    }

    // ペイロード作成
    uint8_t* payload = buffer + HEADER_SIZE;
    memcpy(payload, &data.encoderCountL, 4);
    memcpy(payload + 4, &data.encoderCountR, 4);
    memcpy(payload + 8, &data.targetRpmL, 4);
    memcpy(payload + 12, &data.targetRpmR, 4);
    memcpy(payload + 16, &data.currentRpmL, 4);
    memcpy(payload + 20, &data.currentRpmR, 4);
    memcpy(payload + 24, &data.pwmDutyL, 4);
    memcpy(payload + 28, &data.pwmDutyR, 4);

    // ヘッダ作成
    uint16_t checksum = calculateChecksum(payload, PAYLOAD_LENGTH);
    writeHeader(buffer, REQUEST_GET_DEBUG_OUTPUT, PAYLOAD_LENGTH, checksum);

    return PACKET_LENGTH;
}

uint8_t createSetConfigResponse(uint8_t result, uint8_t* buffer, size_t bufferSize) {
    constexpr uint8_t PAYLOAD_LENGTH = 1;
    constexpr uint8_t PACKET_LENGTH = HEADER_SIZE + PAYLOAD_LENGTH;

    if (bufferSize < PACKET_LENGTH) {
        return 0;
    }

    // ペイロード作成
    uint8_t* payload = buffer + HEADER_SIZE;
    payload[0] = result;

    // ヘッダ作成
    uint16_t checksum = calculateChecksum(payload, PAYLOAD_LENGTH);
    writeHeader(buffer, REQUEST_SET_CONFIG, PAYLOAD_LENGTH, checksum);

    return PACKET_LENGTH;
}

}  // namespace Protocol
