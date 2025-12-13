/**
 * @file MotorController.h
 * @brief モータ制御統合クラス
 *
 * DifferentialKinematics、QuadratureEncoder、PIDController、MotorDriverを統合し、
 * Core1で制御ループを実行する。
 */

#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include "DifferentialKinematics.h"

// 前方宣言（実機用）
class QuadratureEncoder;
class MotorDriver;
class PidController;

/**
 * @class MotorController
 * @brief 差動二輪モータ制御クラス
 *
 * 使用例（実機用）:
 * @code
 * MotorController controller(
 *     encoderL, encoderR, driverL, driverR, pidL, pidR,
 *     0.1f, 0.3f, 1.0f, 200.0f
 * );
 * controller.setCmdVel(0.1f, 0.5f);
 * controller.update(0.01f);
 * @endcode
 *
 * 使用例（テスト用、ロジックのみ）:
 * @code
 * MotorController controller(0.1f, 0.3f, 1.0f, 200.0f);
 * controller.setCmdVel(0.1f, 0.5f);
 * float leftRpm = controller.getTargetRpmL();
 * @endcode
 */
class MotorController {
public:
    /**
     * @brief コンストラクタ（実機用、ハードウェア統合）
     * @param encoderL 左エンコーダ
     * @param encoderR 右エンコーダ
     * @param driverL 左モータドライバ
     * @param driverR 右モータドライバ
     * @param pidL 左PIDコントローラ
     * @param pidR 右PIDコントローラ
     * @param wheelDiameter ホイール直径 [m]
     * @param trackWidth トレッド幅 [m]
     * @param gearRatio 減速比
     * @param maxRpm 最大RPM
     */
    MotorController(
        QuadratureEncoder& encoderL, QuadratureEncoder& encoderR,
        MotorDriver& driverL, MotorDriver& driverR,
        PidController& pidL, PidController& pidR,
        float wheelDiameter, float trackWidth, float gearRatio, float maxRpm
    );

    /**
     * @brief コンストラクタ（テスト用、ロジックのみ）
     * @param wheelDiameter ホイール直径 [m]
     * @param trackWidth トレッド幅 [m]
     * @param gearRatio 減速比
     * @param maxRpm 最大RPM
     */
    MotorController(float wheelDiameter, float trackWidth, float gearRatio, float maxRpm);

    /**
     * @brief cmd_velから目標RPMを計算（回転優先クランプ適用）
     * @param linearX 並進速度 [m/s]
     * @param angularZ 回転速度 [rad/s]
     */
    void setCmdVel(float linearX, float angularZ);

    /**
     * @brief 制御ループを1回実行
     *
     * エンコーダから現在RPMを取得し、PID制御で出力を計算し、
     * モータドライバに出力する。
     *
     * @param dt 前回からの経過時間 [s]
     */
    void update(float dt);

    /**
     * @brief モータを停止
     */
    void stop();

    // --- 目標値 ---
    float getTargetRpmL() const;
    float getTargetRpmR() const;

    // --- 現在値（エンコーダから取得）---
    float getCurrentRpmL() const;
    float getCurrentRpmR() const;
    long getEncoderCountL() const;
    long getEncoderCountR() const;

private:
    /**
     * @brief 回転優先クランプ
     */
    void clampRpmRotationPriority(float& leftRpm, float& rightRpm);

    DifferentialKinematics kinematics_;
    float maxRpm_;
    float targetRpmL_;
    float targetRpmR_;
    float currentRpmL_;
    float currentRpmR_;

    // ハードウェア参照（nullptrの場合はテストモード）
    QuadratureEncoder* encoderL_;
    QuadratureEncoder* encoderR_;
    MotorDriver* driverL_;
    MotorDriver* driverR_;
    PidController* pidL_;
    PidController* pidR_;
};

#endif // MOTOR_CONTROLLER_H
