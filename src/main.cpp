/**
 * @file main.cpp
 * @brief 汎用DCモータ差動二輪コントローラ ファームウェア
 *
 * デュアルコア構成:
 * - Core0: ROS通信、コマンド解析、フェイルセーフ
 * - Core1: エンコーダ読み取り、PID制御、PWM出力
 */

#include <Arduino.h>
#include <PacketSerial.h>

#include "main.h"
#include "Protocol.h"
#include "MotorController.h"
#include "QuadratureEncoder.h"
#include "MotorDriver.h"
#include "PidController.h"

#ifdef DEBUG_BUILD
#include "DebugLogger.h"
#endif

// =============================================================================
// グローバル変数定義
// =============================================================================

// 共有データ（コア間通信）
volatile CmdVelData cmdVelData;
volatile MotorStateData motorStateData;

// 設定・ステータス
RobotConfig config;
SystemStatus systemStatus;

// フェイルセーフ
unsigned long lastCommandTimeMs = 0;

// Core0用
PacketSerial packetSerial;

// Core1用ハードウェアオブジェクト（グローバル直接宣言）
QuadratureEncoder encoderL(
    HardwareConfig::ENCODER_L_A,
    HardwareConfig::ENCODER_L_B,
    HardwareConfig::Defaults::ENCODER_PPR
);
QuadratureEncoder encoderR(
    HardwareConfig::ENCODER_R_A,
    HardwareConfig::ENCODER_R_B,
    HardwareConfig::Defaults::ENCODER_PPR
);

// 右モータは反転（差動二輪のため）
MotorDriver driverL(
    HardwareConfig::MOTOR_L_DIR,
    HardwareConfig::MOTOR_L_PWM,
    false  // 反転なし
);
MotorDriver driverR(
    HardwareConfig::MOTOR_R_DIR,
    HardwareConfig::MOTOR_R_PWM,
    true   // 反転あり
);

PidController pidL(
    HardwareConfig::Defaults::PID_KP,
    HardwareConfig::Defaults::PID_KI,
    HardwareConfig::Defaults::PID_KD
);
PidController pidR(
    HardwareConfig::Defaults::PID_KP,
    HardwareConfig::Defaults::PID_KI,
    HardwareConfig::Defaults::PID_KD
);

MotorController motorController(
    encoderL, encoderR,
    driverL, driverR,
    pidL, pidR,
    0.1f,   // wheelDiameter [m]
    0.3f,   // trackWidth [m]
    HardwareConfig::Defaults::GEAR_RATIO,
    HardwareConfig::Defaults::MAX_RPM
);

// =============================================================================
// プロトコルハンドラ
// =============================================================================

/**
 * MOTOR_COMMANDハンドラ
 */
void handleMotorCommand(const Protocol::ParsedRequest& req) {
    // 共有メモリに書き込み
    cmdVelData.linearX = req.motorCommand.linearX;
    cmdVelData.angularZ = req.motorCommand.angularZ;
    cmdVelData.failsafeStop = false;

    // フェイルセーフタイマーリセット
    lastCommandTimeMs = millis();
    systemStatus.flags &= ~Protocol::STATUS_FAILSAFE;

    // レスポンス作成
    Protocol::MotorCommandResponse resp;
    resp.encoderCountL = motorStateData.encoderCountL;
    resp.encoderCountR = motorStateData.encoderCountR;
    resp.status = systemStatus.flags;

    uint8_t buffer[32];
    uint8_t length = Protocol::createMotorCommandResponse(resp, buffer, sizeof(buffer));
    packetSerial.send(buffer, length);
}

/**
 * GET_VERSIONハンドラ
 */
void handleGetVersion() {
    Protocol::VersionResponse resp;
    resp.major = HardwareConfig::Version::MAJOR;
    resp.minor = HardwareConfig::Version::MINOR;
    resp.patch = HardwareConfig::Version::PATCH;

    uint8_t buffer[16];
    uint8_t length = Protocol::createVersionResponse(resp, buffer, sizeof(buffer));
    packetSerial.send(buffer, length);
}

/**
 * GET_STATUSハンドラ
 */
void handleGetStatus() {
    Protocol::StatusResponse resp;
    resp.status = systemStatus.flags;
    resp.errorCode = systemStatus.lastErrorCode;
    resp.commErrorCount = systemStatus.commErrorCount;
    resp.uptimeMs = millis();

    uint8_t buffer[32];
    uint8_t length = Protocol::createStatusResponse(resp, buffer, sizeof(buffer));
    packetSerial.send(buffer, length);
}

/**
 * GET_CONFIGハンドラ
 */
void handleGetConfig() {
    Protocol::ConfigData resp;
    resp.pidKp = config.pidKp;
    resp.pidKi = config.pidKi;
    resp.pidKd = config.pidKd;
    resp.maxRpm = config.maxRpm;
    resp.encoderPpr = config.encoderPpr;
    resp.gearRatio = config.gearRatio;
    resp.wheelDiameter = config.wheelDiameter;
    resp.trackWidth = config.trackWidth;

    uint8_t buffer[64];
    uint8_t length = Protocol::createConfigResponse(resp, buffer, sizeof(buffer));
    packetSerial.send(buffer, length);
}

/**
 * SET_CONFIGハンドラ
 * TODO: ConfigStorage実装後にFlash保存を追加
 */
void handleSetConfig(const Protocol::ParsedRequest& req) {
    // 設定値を更新
    config.pidKp = req.setConfig.pidKp;
    config.pidKi = req.setConfig.pidKi;
    config.pidKd = req.setConfig.pidKd;
    config.maxRpm = req.setConfig.maxRpm;
    config.encoderPpr = req.setConfig.encoderPpr;
    config.gearRatio = req.setConfig.gearRatio;
    config.wheelDiameter = req.setConfig.wheelDiameter;
    config.trackWidth = req.setConfig.trackWidth;

    // TODO: PIDゲインをCore1のPIDControllerに反映

    uint8_t buffer[16];
    uint8_t length = Protocol::createSetConfigResponse(
        Protocol::CONFIG_RESULT_SUCCESS, buffer, sizeof(buffer));
    packetSerial.send(buffer, length);
}

/**
 * GET_DEBUG_OUTPUTハンドラ
 */
void handleGetDebugOutput() {
    Protocol::DebugOutputResponse resp;
    resp.encoderCountL = motorStateData.encoderCountL;
    resp.encoderCountR = motorStateData.encoderCountR;
    resp.targetRpmL = motorStateData.targetRpmL;
    resp.targetRpmR = motorStateData.targetRpmR;
    resp.currentRpmL = motorStateData.currentRpmL;
    resp.currentRpmR = motorStateData.currentRpmR;
    resp.pwmDutyL = 0.0f;  // TODO: MotorDriverから取得
    resp.pwmDutyR = 0.0f;

    uint8_t buffer[64];
    uint8_t length = Protocol::createDebugOutputResponse(resp, buffer, sizeof(buffer));
    packetSerial.send(buffer, length);
}

/**
 * パケット受信コールバック
 */
void onSerialPacketReceived(const uint8_t* buffer, size_t size) {
    Protocol::ParsedRequest req;
    Protocol::ParseResult result = Protocol::parseRequest(buffer, size, req);

    if (result != Protocol::PARSE_OK) {
        systemStatus.commErrorCount++;
        switch (result) {
            case Protocol::PARSE_ERROR_SIZE:
                systemStatus.lastErrorCode = Protocol::ERROR_PAYLOAD;
                break;
            case Protocol::PARSE_ERROR_CHECKSUM:
                systemStatus.lastErrorCode = Protocol::ERROR_CHECKSUM;
                break;
            case Protocol::PARSE_ERROR_INVALID_TYPE:
                systemStatus.lastErrorCode = Protocol::ERROR_INVALID_COMMAND;
                break;
            default:
                break;
        }
        return;
    }

    // リクエストタイプに応じて処理
    switch (req.requestType) {
        case Protocol::REQUEST_MOTOR_COMMAND:
            handleMotorCommand(req);
            break;
        case Protocol::REQUEST_GET_VERSION:
            handleGetVersion();
            break;
        case Protocol::REQUEST_GET_STATUS:
            handleGetStatus();
            break;
        case Protocol::REQUEST_GET_CONFIG:
            handleGetConfig();
            break;
        case Protocol::REQUEST_SET_CONFIG:
            handleSetConfig(req);
            break;
        case Protocol::REQUEST_GET_DEBUG_OUTPUT:
            handleGetDebugOutput();
            break;
        default:
            break;
    }
}

/**
 * フェイルセーフチェック
 */
void checkFailsafe() {
    unsigned long elapsed = millis() - lastCommandTimeMs;
    if (elapsed > HardwareConfig::FAILSAFE_TIMEOUT_MS) {
        systemStatus.flags |= Protocol::STATUS_FAILSAFE;
        cmdVelData.linearX = 0.0f;
        cmdVelData.angularZ = 0.0f;
        cmdVelData.failsafeStop = true;
    }
}

// =============================================================================
// Core0: メインコア（ROS通信）
// =============================================================================

void setup() {
    // 共有データ初期化
    initCmdVelData(&cmdVelData);
    initMotorStateData(&motorStateData);

    // PacketSerial初期化
    packetSerial.begin(115200);
    packetSerial.setStream(&Serial);
    packetSerial.setPacketHandler(&onSerialPacketReceived);

    // フェイルセーフタイマー初期化
    lastCommandTimeMs = millis();

#ifdef DEBUG_BUILD
    DEBUG_INIT();
    DEBUG_PRINTLN("Core0: Setup complete");
#endif

    // Serialバッファをクリア
    delay(100);
    while (Serial.available() > 0) {
        Serial.read();
    }
}

void loop() {
    static unsigned long prevTimeUs = 0;
    unsigned long currentUs = micros();

    // PacketSerial更新（受信処理）- 毎ループ実行
    packetSerial.update();

    // オーバーフローチェック
    if (packetSerial.overflow()) {
        systemStatus.commErrorCount++;
    }

    // 100msごとの処理
    if (currentUs - prevTimeUs >= 100000) {
        prevTimeUs = currentUs;
        checkFailsafe();
    }
}

// =============================================================================
// Core1: リアルタイムコア（モータ制御）
// =============================================================================

void setup1() {
    // PID出力リミット設定
    pidL.setOutputLimits(-1.0f, 1.0f);
    pidR.setOutputLimits(-1.0f, 1.0f);

    // ハードウェア初期化
    encoderL.begin();
    encoderR.begin();
    driverL.begin();
    driverR.begin();

#ifdef DEBUG_BUILD
    DEBUG_PRINTLN("Core1: Setup complete");
#endif
}

void loop1() {
    static unsigned long prevTimeUs = 0;
    unsigned long currentUs = micros();

    // 制御周期（10ms = 100Hz）
    if (currentUs - prevTimeUs >= HardwareConfig::CONTROL_PERIOD_US) {
        float dt = (currentUs - prevTimeUs) / 1000000.0f;
        prevTimeUs = currentUs;

        // 共有メモリからcmd_velを読み込み
        float linearX = cmdVelData.linearX;
        float angularZ = cmdVelData.angularZ;
        bool failsafe = cmdVelData.failsafeStop;

        // フェイルセーフ時は停止
        if (failsafe) {
            motorController.stop();
        } else {
            // cmd_velを設定して制御ループ実行
            motorController.setCmdVel(linearX, angularZ);
            motorController.update(dt);
        }

        // 共有メモリに状態を書き込み
        motorStateData.encoderCountL = motorController.getEncoderCountL();
        motorStateData.encoderCountR = motorController.getEncoderCountR();
        motorStateData.targetRpmL = motorController.getTargetRpmL();
        motorStateData.targetRpmR = motorController.getTargetRpmR();
        motorStateData.currentRpmL = motorController.getCurrentRpmL();
        motorStateData.currentRpmR = motorController.getCurrentRpmR();

#ifdef DEBUG_BUILD
        static int debugCounter = 0;
        if (++debugCounter >= 100) {  // 1秒ごと
            DEBUG_PRINTF("RPM: L=%.1f/%.1f R=%.1f/%.1f\n",
                motorStateData.currentRpmL, motorStateData.targetRpmL,
                motorStateData.currentRpmR, motorStateData.targetRpmR);
            debugCounter = 0;
        }
#endif
    }
}
