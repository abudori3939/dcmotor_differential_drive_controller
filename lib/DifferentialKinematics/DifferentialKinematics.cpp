/**
 * @file DifferentialKinematics.cpp
 * @brief 差動二輪キネマティクス計算 実装
 */

#include "DifferentialKinematics.h"

namespace {
    constexpr float PI = 3.14159f;
}

DifferentialKinematics::DifferentialKinematics(float wheelDiameter, float trackWidth, float gearRatio)
    : wheelDiameter_(wheelDiameter)
    , trackWidth_(trackWidth)
    , gearRatio_(gearRatio)
{
}

void DifferentialKinematics::calculate(float linearX, float angularZ, float& leftRpm, float& rightRpm) const {
    float wheelRadius = wheelDiameter_ / 2.0f;

    // 左右ホイールの速度 [m/s]
    float leftVel = linearX - angularZ * trackWidth_ / 2.0f;
    float rightVel = linearX + angularZ * trackWidth_ / 2.0f;

    // ホイール速度 [m/s] → モータRPM
    // RPM = vel / (2 * PI * r) * 60 * gear_ratio
    float velToRpm = 60.0f / (2.0f * PI * wheelRadius) * gearRatio_;
    leftRpm = leftVel * velToRpm;
    rightRpm = rightVel * velToRpm;
}
