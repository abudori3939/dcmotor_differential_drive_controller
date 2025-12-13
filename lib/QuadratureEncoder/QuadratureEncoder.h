/**
 * QuadratureEncoder - 2相エンコーダ読み取りライブラリ
 *
 * 4逓倍デコードによる高精度カウント。
 * ハードウェア非依存のロジック部分（calculateRpm, decodeState）と
 * ハードウェア依存部分（begin, 割り込みハンドラ）を分離。
 */

#ifndef QUADRATURE_ENCODER_H
#define QUADRATURE_ENCODER_H

#include <stdint.h>

class QuadratureEncoder {
public:
    /**
     * コンストラクタ
     * @param pinA A相ピン番号
     * @param pinB B相ピン番号
     * @param ppr エンコーダのPPR（Pulses Per Revolution）
     */
    QuadratureEncoder(uint8_t pinA, uint8_t pinB, uint16_t ppr);

    /**
     * エンコーダを初期化（割り込み設定）
     * ハードウェア依存のため実機でのみ動作
     */
    void begin();

    /**
     * 累積カウントを取得
     * @return 累積カウント値（正:正転、負:逆転）
     */
    int32_t getCount() const;

    /**
     * カウントをリセット
     */
    void resetCount();

    /**
     * 現在のRPMを取得
     * @param dt 前回呼び出しからの経過時間[秒]
     * @return RPM（正:正転、負:逆転）
     */
    float getRpm(float dt);

    /**
     * RPMを計算（ハードウェア非依存、テスト可能）
     * @param countDiff カウント差分
     * @param ppr PPR（Pulses Per Revolution）
     * @param dt 経過時間[秒]
     * @return RPM
     */
    static float calculateRpm(int32_t countDiff, uint16_t ppr, float dt);

    /**
     * 4逓倍デコード（ハードウェア非依存、テスト可能）
     * A/B相の状態遷移からカウント増減を判定
     * @param prevState 前回の状態 (A << 1) | B
     * @param currState 現在の状態 (A << 1) | B
     * @return カウント増減（+1:正転、-1:逆転、0:変化なしor不正遷移）
     */
    static int8_t decodeState(uint8_t prevState, uint8_t currState);

private:
    uint8_t pinA_;
    uint8_t pinB_;
    uint16_t ppr_;
    volatile int32_t count_;
    int32_t prevCount_;
    uint8_t prevState_;

    // 割り込みハンドラ（実装時に使用）
    void handleInterrupt();
};

#endif // QUADRATURE_ENCODER_H
