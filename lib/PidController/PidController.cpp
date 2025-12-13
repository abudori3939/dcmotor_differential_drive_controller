#include "PidController.h"

PidController::PidController(float kp, float ki, float kd)
    : kp_(kp), ki_(ki), kd_(kd),
      integral_(0.0f), prevError_(0.0f), isFirstCall_(true),
      outputMin_(0.0f), outputMax_(0.0f), hasOutputLimits_(false) {
}

float PidController::compute(float setpoint, float measured, float dt) {
    // dtのガード: 0以下なら計算不可
    if (dt <= 0.0f) {
        return 0.0f;
    }

    // 誤差計算
    float error = setpoint - measured;

    // P項
    float pTerm = kp_ * error;

    // D項（初回は0、前回誤差を現在誤差で初期化）
    float dTerm = 0.0f;
    if (isFirstCall_) {
        prevError_ = error;
        isFirstCall_ = false;
    } else {
        dTerm = kd_ * (error - prevError_) / dt;
    }
    prevError_ = error;

    // アンチワインドアップ判定用の仮出力計算（積分更新前）
    float preIntegral = ki_ * integral_;
    float preOutput = pTerm + preIntegral + dTerm;

    // アンチワインドアップ: 条件付き積分（改良版）
    // 飽和中かつ誤差と出力が同じ方向の場合は積分を停止
    bool shouldIntegrate = true;
    if (hasOutputLimits_) {
        if ((preOutput > outputMax_ && error > 0.0f) ||
            (preOutput < outputMin_ && error < 0.0f)) {
            shouldIntegrate = false;
        }
    }

    if (shouldIntegrate) {
        integral_ += error * dt;
    }

    // I項計算（積分更新後）
    float iTerm = ki_ * integral_;
    float rawOutput = pTerm + iTerm + dTerm;

    // 出力リミット適用
    float output = rawOutput;
    if (hasOutputLimits_) {
        if (rawOutput > outputMax_) {
            output = outputMax_;
        } else if (rawOutput < outputMin_) {
            output = outputMin_;
        }
    }

    return output;
}

void PidController::setGains(float kp, float ki, float kd) {
    kp_ = kp;
    ki_ = ki;
    kd_ = kd;
}

void PidController::setOutputLimits(float min, float max) {
    outputMin_ = min;
    outputMax_ = max;
    hasOutputLimits_ = true;
}

void PidController::reset() {
    integral_ = 0.0f;
    prevError_ = 0.0f;
    isFirstCall_ = true;
}
