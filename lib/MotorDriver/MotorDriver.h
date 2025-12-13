#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include <stdint.h>

/**
 * MotorDriver - 汎用DCモータドライバ（方向+PWM方式）
 *
 * モータドライバの仕様:
 * - DIRピン: LOW=正転、HIGH=逆転
 * - PWMピン: 0〜255（8bit）でデューティサイクル制御
 */
class MotorDriver {
public:
    /**
     * コンストラクタ
     * @param pinDir 方向ピン番号
     * @param pinPwm PWMピン番号
     */
    MotorDriver(uint8_t pinDir, uint8_t pinPwm);

    /**
     * 初期化（ピンモード設定、PWM周波数設定）
     */
    void begin();

    /**
     * 速度設定
     * @param speed 速度（-1.0〜1.0、負で逆転）
     */
    void setSpeed(float speed);

    /**
     * 停止（PWMを0に）
     */
    void stop();

    /**
     * ブレーキ（ドライバ対応時のみ）
     */
    void brake();

    // =========================================================================
    // 静的ユーティリティ関数（テスト可能なロジック部分）
    // =========================================================================

    /**
     * 速度値を-1.0〜1.0にクランプ
     * @param speed 入力速度
     * @return クランプされた速度
     */
    static float clampSpeed(float speed);

    /**
     * 速度から方向を判定
     * @param speed 速度（正:正転、負:逆転）
     * @return 方向（false=LOW=正転、true=HIGH=逆転）
     */
    static bool getDirection(float speed);

    /**
     * 速度からPWMデューティサイクルを計算
     * @param speed 速度（-1.0〜1.0）
     * @return PWMデューティサイクル（0〜255）
     */
    static uint8_t calculatePwmDuty(float speed);

    // =========================================================================
    // 定数
    // =========================================================================
    static constexpr uint8_t PWM_MAX = 255;

private:
    uint8_t pinDir_;
    uint8_t pinPwm_;
    float currentSpeed_;
};

#endif // MOTOR_DRIVER_H
