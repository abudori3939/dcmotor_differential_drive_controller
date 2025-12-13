/**
 * @file MotorController.cpp
 * @brief モータ制御統合クラス 実装
 */

#include "MotorController.h"
#include "QuadratureEncoder.h"
#include "MotorDriver.h"
#include "PidController.h"
#include <algorithm>
#include <cmath>

// 実機用コンストラクタ
MotorController::MotorController(
    QuadratureEncoder& encoderL, QuadratureEncoder& encoderR,
    MotorDriver& driverL, MotorDriver& driverR,
    PidController& pidL, PidController& pidR,
    float wheelDiameter, float trackWidth, float gearRatio, float maxRpm
)
    : kinematics_(wheelDiameter, trackWidth, gearRatio)
    , maxRpm_(maxRpm)
    , targetRpmL_(0.0f)
    , targetRpmR_(0.0f)
    , currentRpmL_(0.0f)
    , currentRpmR_(0.0f)
    , encoderL_(&encoderL)
    , encoderR_(&encoderR)
    , driverL_(&driverL)
    , driverR_(&driverR)
    , pidL_(&pidL)
    , pidR_(&pidR)
{
}

// テスト用コンストラクタ（ロジックのみ）
MotorController::MotorController(float wheelDiameter, float trackWidth, float gearRatio, float maxRpm)
    : kinematics_(wheelDiameter, trackWidth, gearRatio)
    , maxRpm_(maxRpm)
    , targetRpmL_(0.0f)
    , targetRpmR_(0.0f)
    , currentRpmL_(0.0f)
    , currentRpmR_(0.0f)
    , encoderL_(nullptr)
    , encoderR_(nullptr)
    , driverL_(nullptr)
    , driverR_(nullptr)
    , pidL_(nullptr)
    , pidR_(nullptr)
{
}

void MotorController::setCmdVel(float linearX, float angularZ) {
    // キネマティクス計算で目標RPMを算出
    kinematics_.calculate(linearX, angularZ, targetRpmL_, targetRpmR_);

    // 回転優先クランプを適用
    clampRpmRotationPriority(targetRpmL_, targetRpmR_);
}

void MotorController::update(float dt) {
    // ハードウェアが接続されていない場合は何もしない
    if (encoderL_ == nullptr || encoderR_ == nullptr ||
        driverL_ == nullptr || driverR_ == nullptr ||
        pidL_ == nullptr || pidR_ == nullptr) {
        return;
    }

    // エンコーダから現在RPMを取得
    currentRpmL_ = encoderL_->getRpm(dt);
    currentRpmR_ = encoderR_->getRpm(dt);

    // PID制御で出力を計算
    float outputL = pidL_->compute(targetRpmL_, currentRpmL_, dt);
    float outputR = pidR_->compute(targetRpmR_, currentRpmR_, dt);

    // モータドライバに出力（-1.0〜1.0に正規化）
    float normalizedL = outputL / maxRpm_;
    float normalizedR = outputR / maxRpm_;
    driverL_->setSpeed(normalizedL);
    driverR_->setSpeed(normalizedR);
}

void MotorController::stop() {
    targetRpmL_ = 0.0f;
    targetRpmR_ = 0.0f;

    if (driverL_ != nullptr && driverR_ != nullptr) {
        driverL_->stop();
        driverR_->stop();
    }

    if (pidL_ != nullptr && pidR_ != nullptr) {
        pidL_->reset();
        pidR_->reset();
    }
}

float MotorController::getTargetRpmL() const {
    return targetRpmL_;
}

float MotorController::getTargetRpmR() const {
    return targetRpmR_;
}

float MotorController::getCurrentRpmL() const {
    return currentRpmL_;
}

float MotorController::getCurrentRpmR() const {
    return currentRpmR_;
}

long MotorController::getEncoderCountL() const {
    if (encoderL_ == nullptr) {
        return 0;
    }
    return encoderL_->getCount();
}

long MotorController::getEncoderCountR() const {
    if (encoderR_ == nullptr) {
        return 0;
    }
    return encoderR_->getCount();
}

void MotorController::clampRpmRotationPriority(float& leftRpm, float& rightRpm) {
    // 目標RPMを並進成分(vTrans)と回転成分(vRot)に分解
    float vTrans = (rightRpm + leftRpm) / 2.0f;
    float vRot = (rightRpm - leftRpm) / 2.0f;

    // 回転成分をmax_rpmでクランプ
    float clampedVRot = std::max(-maxRpm_, std::min(maxRpm_, vRot));

    // 回転を維持するために、並進の上限を計算
    float vTransLimit = maxRpm_ - std::abs(clampedVRot);

    // 並進成分を上限値でクランプ
    float clampedVTrans = std::max(-vTransLimit, std::min(vTransLimit, vTrans));

    // 最終的なRPMを再計算
    leftRpm = clampedVTrans - clampedVRot;
    rightRpm = clampedVTrans + clampedVRot;
}
