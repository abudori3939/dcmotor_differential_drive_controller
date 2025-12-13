#include <unity.h>
#include "Protocol.h"
#include <cstring>

void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// チェックサム計算テスト
// ============================================================================

void test_checksum_empty_payload(void) {
    // 空のペイロード
    uint8_t payload[] = {};
    uint16_t checksum = Protocol::calculateChecksum(payload, 0);
    TEST_ASSERT_EQUAL_UINT16(0, checksum);
}

void test_checksum_single_byte(void) {
    // 1バイト
    uint8_t payload[] = {0x42};
    uint16_t checksum = Protocol::calculateChecksum(payload, 1);
    TEST_ASSERT_EQUAL_UINT16(0x42, checksum);
}

void test_checksum_multiple_bytes(void) {
    // 複数バイト（0x01 + 0x02 + 0x03 + 0x04 = 0x0A）
    uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};
    uint16_t checksum = Protocol::calculateChecksum(payload, 4);
    TEST_ASSERT_EQUAL_UINT16(0x0A, checksum);
}

void test_checksum_large_sum(void) {
    // 大きな値の加算（0xFF * 3 = 765 = 0x02FD）
    uint8_t payload[] = {0xFF, 0xFF, 0xFF};
    uint16_t checksum = Protocol::calculateChecksum(payload, 3);
    TEST_ASSERT_EQUAL_UINT16(0x02FD, checksum);
}

void test_checksum_16bit_wrap(void) {
    // 16bitオーバーフロー（0xFFFF + 1 = 0x0000）
    // 256個の0xFFで256 * 255 = 65280、さらに256個で合計130560
    // 130560 & 0xFFFF = 130560 - 65536 * 2 = -512 → 65024 = 0xFE00
    // より簡単なケース: 合計65536になるパターン
    // 0x80 * 512 = 65536 → wrap to 0
    // 実用的なテスト: 0xFF + 0xFF + ... (257個) = 65535 + 255 = 65790 → 254
    uint8_t payload[258];
    memset(payload, 0xFF, 258);  // 258 * 255 = 65790 → 65790 & 0xFFFF = 254
    uint16_t checksum = Protocol::calculateChecksum(payload, 258);
    TEST_ASSERT_EQUAL_UINT16(254, checksum);
}

// ============================================================================
// リクエストパーステスト
// ============================================================================

void test_parse_motor_command_request(void) {
    // MOTOR_COMMANDリクエスト（linear_x=0.5, angular_z=1.0）
    float linearX = 0.5f;
    float angularZ = 1.0f;
    uint8_t payload[8];
    memcpy(payload, &linearX, 4);
    memcpy(payload + 4, &angularZ, 4);
    uint16_t checksum = Protocol::calculateChecksum(payload, 8);

    uint8_t packet[12];
    packet[0] = Protocol::REQUEST_MOTOR_COMMAND;
    packet[1] = 8;  // payload length
    packet[2] = checksum & 0xFF;
    packet[3] = (checksum >> 8) & 0xFF;
    memcpy(packet + 4, payload, 8);

    Protocol::ParsedRequest req;
    Protocol::ParseResult result = Protocol::parseRequest(packet, 12, req);

    TEST_ASSERT_EQUAL(Protocol::PARSE_OK, result);
    TEST_ASSERT_EQUAL_UINT8(Protocol::REQUEST_MOTOR_COMMAND, req.requestType);
    TEST_ASSERT_EQUAL_UINT8(8, req.payloadLength);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, req.motorCommand.linearX);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, req.motorCommand.angularZ);
}

void test_parse_get_version_request(void) {
    // GET_VERSIONリクエスト（ペイロードなし）
    uint8_t packet[4] = {
        Protocol::REQUEST_GET_VERSION,
        0,  // payload length
        0, 0  // checksum (0 for empty payload)
    };

    Protocol::ParsedRequest req;
    Protocol::ParseResult result = Protocol::parseRequest(packet, 4, req);

    TEST_ASSERT_EQUAL(Protocol::PARSE_OK, result);
    TEST_ASSERT_EQUAL_UINT8(Protocol::REQUEST_GET_VERSION, req.requestType);
    TEST_ASSERT_EQUAL_UINT8(0, req.payloadLength);
}

void test_parse_invalid_checksum(void) {
    // チェックサム不正
    float linearX = 0.5f;
    float angularZ = 1.0f;
    uint8_t payload[8];
    memcpy(payload, &linearX, 4);
    memcpy(payload + 4, &angularZ, 4);

    uint8_t packet[12];
    packet[0] = Protocol::REQUEST_MOTOR_COMMAND;
    packet[1] = 8;
    packet[2] = 0xFF;  // 不正なチェックサム
    packet[3] = 0xFF;
    memcpy(packet + 4, payload, 8);

    Protocol::ParsedRequest req;
    Protocol::ParseResult result = Protocol::parseRequest(packet, 12, req);

    TEST_ASSERT_EQUAL(Protocol::PARSE_ERROR_CHECKSUM, result);
}

void test_parse_packet_too_short(void) {
    // パケットが短すぎる
    uint8_t packet[2] = {0x00, 0x08};

    Protocol::ParsedRequest req;
    Protocol::ParseResult result = Protocol::parseRequest(packet, 2, req);

    TEST_ASSERT_EQUAL(Protocol::PARSE_ERROR_SIZE, result);
}

void test_parse_payload_length_mismatch(void) {
    // ペイロード長が実際のサイズと不一致
    uint8_t packet[4] = {
        Protocol::REQUEST_MOTOR_COMMAND,
        8,  // 8バイト必要だが実際は0
        0, 0
    };

    Protocol::ParsedRequest req;
    Protocol::ParseResult result = Protocol::parseRequest(packet, 4, req);

    TEST_ASSERT_EQUAL(Protocol::PARSE_ERROR_SIZE, result);
}

void test_parse_invalid_request_type(void) {
    // 不正なリクエストタイプ
    uint8_t packet[4] = {
        0xFE,  // 不正なタイプ
        0,
        0, 0
    };

    Protocol::ParsedRequest req;
    Protocol::ParseResult result = Protocol::parseRequest(packet, 4, req);

    TEST_ASSERT_EQUAL(Protocol::PARSE_ERROR_INVALID_TYPE, result);
}

// ============================================================================
// SET_CONFIGリクエストパーステスト
// ============================================================================

void test_parse_set_config_request(void) {
    Protocol::ConfigData cfg;
    cfg.pidKp = 2.0f;
    cfg.pidKi = 0.2f;
    cfg.pidKd = 0.02f;
    cfg.maxRpm = 150.0f;
    cfg.encoderPpr = 512;
    cfg.gearRatio = 2.0f;
    cfg.wheelDiameter = 0.08f;
    cfg.trackWidth = 0.25f;

    // ペイロード作成
    uint8_t payload[30];
    memcpy(payload, &cfg.pidKp, 4);
    memcpy(payload + 4, &cfg.pidKi, 4);
    memcpy(payload + 8, &cfg.pidKd, 4);
    memcpy(payload + 12, &cfg.maxRpm, 4);
    memcpy(payload + 16, &cfg.encoderPpr, 2);
    memcpy(payload + 18, &cfg.gearRatio, 4);
    memcpy(payload + 22, &cfg.wheelDiameter, 4);
    memcpy(payload + 26, &cfg.trackWidth, 4);

    uint16_t checksum = Protocol::calculateChecksum(payload, 30);

    uint8_t packet[34];
    packet[0] = Protocol::REQUEST_SET_CONFIG;
    packet[1] = 30;
    packet[2] = checksum & 0xFF;
    packet[3] = (checksum >> 8) & 0xFF;
    memcpy(packet + 4, payload, 30);

    Protocol::ParsedRequest req;
    Protocol::ParseResult result = Protocol::parseRequest(packet, 34, req);

    TEST_ASSERT_EQUAL(Protocol::PARSE_OK, result);
    TEST_ASSERT_EQUAL_UINT8(Protocol::REQUEST_SET_CONFIG, req.requestType);
    // 全フィールド検証
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, req.setConfig.pidKp);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.2f, req.setConfig.pidKi);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.02f, req.setConfig.pidKd);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 150.0f, req.setConfig.maxRpm);
    TEST_ASSERT_EQUAL_UINT16(512, req.setConfig.encoderPpr);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, req.setConfig.gearRatio);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.08f, req.setConfig.wheelDiameter);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.25f, req.setConfig.trackWidth);
}

// ============================================================================
// レスポンス作成テスト
// ============================================================================

void test_create_motor_command_response(void) {
    Protocol::MotorCommandResponse data;
    data.encoderCountL = 1000;
    data.encoderCountR = -2000;
    data.status = 0x0001;  // FAILSAFE

    uint8_t buffer[32];
    uint8_t length = Protocol::createMotorCommandResponse(data, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_UINT8(14, length);  // ヘッダ4 + ペイロード10
    TEST_ASSERT_EQUAL_UINT8(Protocol::REQUEST_MOTOR_COMMAND, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(10, buffer[1]);  // payload length

    // 全フィールド検証
    int32_t encL, encR;
    uint16_t status;
    memcpy(&encL, buffer + 4, 4);
    memcpy(&encR, buffer + 8, 4);
    memcpy(&status, buffer + 12, 2);

    TEST_ASSERT_EQUAL_INT32(1000, encL);
    TEST_ASSERT_EQUAL_INT32(-2000, encR);
    TEST_ASSERT_EQUAL_UINT16(0x0001, status);

    // チェックサム検証
    uint16_t receivedChecksum = buffer[2] | (buffer[3] << 8);
    uint16_t calculatedChecksum = Protocol::calculateChecksum(buffer + 4, 10);
    TEST_ASSERT_EQUAL_UINT16(calculatedChecksum, receivedChecksum);
}

void test_create_version_response(void) {
    Protocol::VersionResponse data;
    data.major = 1;
    data.minor = 2;
    data.patch = 3;

    uint8_t buffer[32];
    uint8_t length = Protocol::createVersionResponse(data, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_UINT8(8, length);  // ヘッダ4 + ペイロード4
    TEST_ASSERT_EQUAL_UINT8(Protocol::REQUEST_GET_VERSION, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(4, buffer[1]);

    // 全フィールド検証
    TEST_ASSERT_EQUAL_UINT8(1, buffer[4]);  // major
    TEST_ASSERT_EQUAL_UINT8(2, buffer[5]);  // minor
    TEST_ASSERT_EQUAL_UINT8(3, buffer[6]);  // patch
    TEST_ASSERT_EQUAL_UINT8(0, buffer[7]);  // reserved

    // チェックサム検証
    uint16_t receivedChecksum = buffer[2] | (buffer[3] << 8);
    uint16_t calculatedChecksum = Protocol::calculateChecksum(buffer + 4, 4);
    TEST_ASSERT_EQUAL_UINT16(calculatedChecksum, receivedChecksum);
}

void test_create_status_response(void) {
    Protocol::StatusResponse data;
    data.status = 0x8001;  // CONFIG_MODE | FAILSAFE
    data.errorCode = 0x01;
    data.commErrorCount = 5;
    data.uptimeMs = 123456;

    uint8_t buffer[32];
    uint8_t length = Protocol::createStatusResponse(data, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_UINT8(16, length);  // ヘッダ4 + ペイロード12
    TEST_ASSERT_EQUAL_UINT8(Protocol::REQUEST_GET_STATUS, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(12, buffer[1]);

    // 全フィールド検証
    uint16_t status;
    uint8_t errorCode;
    uint16_t commErrorCount;
    uint32_t uptimeMs;
    memcpy(&status, buffer + 4, 2);
    errorCode = buffer[6];
    // buffer[7] は reserved
    memcpy(&commErrorCount, buffer + 8, 2);
    // buffer[10-11] は reserved
    memcpy(&uptimeMs, buffer + 12, 4);

    TEST_ASSERT_EQUAL_UINT16(0x8001, status);
    TEST_ASSERT_EQUAL_UINT8(0x01, errorCode);
    TEST_ASSERT_EQUAL_UINT16(5, commErrorCount);
    TEST_ASSERT_EQUAL_UINT32(123456, uptimeMs);

    // チェックサム検証
    uint16_t receivedChecksum = buffer[2] | (buffer[3] << 8);
    uint16_t calculatedChecksum = Protocol::calculateChecksum(buffer + 4, 12);
    TEST_ASSERT_EQUAL_UINT16(calculatedChecksum, receivedChecksum);
}

void test_create_config_response(void) {
    Protocol::ConfigData data;
    data.pidKp = 1.0f;
    data.pidKi = 0.1f;
    data.pidKd = 0.01f;
    data.maxRpm = 200.0f;
    data.encoderPpr = 1024;
    data.gearRatio = 1.5f;
    data.wheelDiameter = 0.1f;
    data.trackWidth = 0.3f;

    uint8_t buffer[64];
    uint8_t length = Protocol::createConfigResponse(data, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_UINT8(34, length);  // ヘッダ4 + ペイロード30
    TEST_ASSERT_EQUAL_UINT8(Protocol::REQUEST_GET_CONFIG, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(30, buffer[1]);

    // 全フィールド検証
    float pidKp, pidKi, pidKd, maxRpm, gearRatio, wheelDiameter, trackWidth;
    uint16_t encoderPpr;
    memcpy(&pidKp, buffer + 4, 4);
    memcpy(&pidKi, buffer + 8, 4);
    memcpy(&pidKd, buffer + 12, 4);
    memcpy(&maxRpm, buffer + 16, 4);
    memcpy(&encoderPpr, buffer + 20, 2);
    memcpy(&gearRatio, buffer + 22, 4);
    memcpy(&wheelDiameter, buffer + 26, 4);
    memcpy(&trackWidth, buffer + 30, 4);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, pidKp);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.1f, pidKi);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.01f, pidKd);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 200.0f, maxRpm);
    TEST_ASSERT_EQUAL_UINT16(1024, encoderPpr);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, gearRatio);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.1f, wheelDiameter);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.3f, trackWidth);

    // チェックサム検証
    uint16_t receivedChecksum = buffer[2] | (buffer[3] << 8);
    uint16_t calculatedChecksum = Protocol::calculateChecksum(buffer + 4, 30);
    TEST_ASSERT_EQUAL_UINT16(calculatedChecksum, receivedChecksum);
}

void test_create_debug_output_response(void) {
    Protocol::DebugOutputResponse data;
    data.encoderCountL = 100;
    data.encoderCountR = 200;
    data.targetRpmL = 50.0f;
    data.targetRpmR = 60.0f;
    data.currentRpmL = 48.5f;
    data.currentRpmR = 58.2f;
    data.pwmDutyL = 0.5f;
    data.pwmDutyR = 0.6f;

    uint8_t buffer[64];
    uint8_t length = Protocol::createDebugOutputResponse(data, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_UINT8(36, length);  // ヘッダ4 + ペイロード32
    TEST_ASSERT_EQUAL_UINT8(Protocol::REQUEST_GET_DEBUG_OUTPUT, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(32, buffer[1]);

    // 全フィールド検証
    int32_t encL, encR;
    float targetL, targetR, currentL, currentR, pwmL, pwmR;
    memcpy(&encL, buffer + 4, 4);
    memcpy(&encR, buffer + 8, 4);
    memcpy(&targetL, buffer + 12, 4);
    memcpy(&targetR, buffer + 16, 4);
    memcpy(&currentL, buffer + 20, 4);
    memcpy(&currentR, buffer + 24, 4);
    memcpy(&pwmL, buffer + 28, 4);
    memcpy(&pwmR, buffer + 32, 4);

    TEST_ASSERT_EQUAL_INT32(100, encL);
    TEST_ASSERT_EQUAL_INT32(200, encR);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, targetL);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 60.0f, targetR);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 48.5f, currentL);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 58.2f, currentR);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, pwmL);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.6f, pwmR);

    // チェックサム検証
    uint16_t receivedChecksum = buffer[2] | (buffer[3] << 8);
    uint16_t calculatedChecksum = Protocol::calculateChecksum(buffer + 4, 32);
    TEST_ASSERT_EQUAL_UINT16(calculatedChecksum, receivedChecksum);
}

void test_create_set_config_response_success(void) {
    uint8_t buffer[16];
    uint8_t length = Protocol::createSetConfigResponse(Protocol::CONFIG_RESULT_SUCCESS, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_UINT8(5, length);  // ヘッダ4 + ペイロード1
    TEST_ASSERT_EQUAL_UINT8(Protocol::REQUEST_SET_CONFIG, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(1, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[4]);  // SUCCESS

    // チェックサム検証
    uint16_t receivedChecksum = buffer[2] | (buffer[3] << 8);
    uint16_t calculatedChecksum = Protocol::calculateChecksum(buffer + 4, 1);
    TEST_ASSERT_EQUAL_UINT16(calculatedChecksum, receivedChecksum);
}

void test_create_set_config_response_error(void) {
    uint8_t buffer[16];
    uint8_t length = Protocol::createSetConfigResponse(Protocol::CONFIG_RESULT_FLASH_ERROR, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_UINT8(5, length);
    TEST_ASSERT_EQUAL_UINT8(Protocol::REQUEST_SET_CONFIG, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[4]);  // FLASH_ERROR
}

// ============================================================================
// メイン
// ============================================================================

int main(void) {
    UNITY_BEGIN();

    // チェックサム
    RUN_TEST(test_checksum_empty_payload);
    RUN_TEST(test_checksum_single_byte);
    RUN_TEST(test_checksum_multiple_bytes);
    RUN_TEST(test_checksum_large_sum);
    RUN_TEST(test_checksum_16bit_wrap);

    // リクエストパース
    RUN_TEST(test_parse_motor_command_request);
    RUN_TEST(test_parse_get_version_request);
    RUN_TEST(test_parse_invalid_checksum);
    RUN_TEST(test_parse_packet_too_short);
    RUN_TEST(test_parse_payload_length_mismatch);
    RUN_TEST(test_parse_invalid_request_type);
    RUN_TEST(test_parse_set_config_request);

    // レスポンス作成
    RUN_TEST(test_create_motor_command_response);
    RUN_TEST(test_create_version_response);
    RUN_TEST(test_create_status_response);
    RUN_TEST(test_create_config_response);
    RUN_TEST(test_create_debug_output_response);
    RUN_TEST(test_create_set_config_response_success);
    RUN_TEST(test_create_set_config_response_error);

    return UNITY_END();
}
