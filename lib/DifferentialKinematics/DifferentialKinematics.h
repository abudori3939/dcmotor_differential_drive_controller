/**
 * @file DifferentialKinematics.h
 * @brief 差動二輪キネマティクス計算
 *
 * cmd_vel（linear_x, angular_z）から左右ホイールRPMを計算する
 */

#ifndef DIFFERENTIAL_KINEMATICS_H
#define DIFFERENTIAL_KINEMATICS_H

/**
 * @class DifferentialKinematics
 * @brief 差動二輪ロボットのキネマティクス計算クラス
 *
 * 使用例:
 * @code
 * DifferentialKinematics kinematics(0.1f, 0.3f, 1.0f);
 * float leftRpm, rightRpm;
 * kinematics.calculate(0.1f, 0.5f, leftRpm, rightRpm);
 * @endcode
 */
class DifferentialKinematics {
public:
    /**
     * @brief コンストラクタ
     * @param wheelDiameter ホイール直径 [m]
     * @param trackWidth トレッド幅（左右ホイール間距離）[m]
     * @param gearRatio 減速比（モータ軸→ホイール軸）
     */
    DifferentialKinematics(float wheelDiameter, float trackWidth, float gearRatio);

    /**
     * @brief cmd_velから左右ホイールRPMを計算
     * @param linearX 並進速度 [m/s]
     * @param angularZ 回転速度 [rad/s]（正で左旋回）
     * @param[out] leftRpm 左ホイールRPM
     * @param[out] rightRpm 右ホイールRPM
     */
    void calculate(float linearX, float angularZ, float& leftRpm, float& rightRpm) const;

private:
    float wheelDiameter_;
    float trackWidth_;
    float gearRatio_;
};

#endif // DIFFERENTIAL_KINEMATICS_H
