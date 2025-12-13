#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

/**
 * PID制御クラス
 *
 * 特徴:
 * - アンチワインドアップ: 条件付き積分（出力飽和中は誤差方向の積分を停止）
 * - 初回/reset後のD項: 0（前回誤差を現在誤差で初期化）
 * - dt <= 0 のガード: 0.0fを返す
 */
class PidController {
public:
    /**
     * コンストラクタ
     * @param kp 比例ゲイン
     * @param ki 積分ゲイン
     * @param kd 微分ゲイン
     */
    PidController(float kp, float ki, float kd);

    /**
     * PID制御出力を計算
     * @param setpoint 目標値（RPM）
     * @param measured 現在値（RPM、エンコーダから取得）
     * @param dt 時間刻み（秒）
     * @return 制御出力（リミット適用後）
     */
    float compute(float setpoint, float measured, float dt);

    /**
     * ゲインを設定
     * @param kp 比例ゲイン
     * @param ki 積分ゲイン
     * @param kd 微分ゲイン
     */
    void setGains(float kp, float ki, float kd);

    /**
     * 出力リミットを設定
     * @param min 最小出力
     * @param max 最大出力
     */
    void setOutputLimits(float min, float max);

    /**
     * 内部状態をリセット（積分値、前回誤差）
     */
    void reset();

private:
    float kp_;
    float ki_;
    float kd_;

    float integral_;
    float prevError_;
    bool isFirstCall_;

    float outputMin_;
    float outputMax_;
    bool hasOutputLimits_;
};

#endif  // PID_CONTROLLER_H
