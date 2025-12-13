/**
 * QuadratureEncoder 実装
 *
 * スケルトン実装（TDD Red phase）
 */

#include "QuadratureEncoder.h"

QuadratureEncoder::QuadratureEncoder(uint8_t pinA, uint8_t pinB, uint16_t ppr)
    : pinA_(pinA), pinB_(pinB), ppr_(ppr), count_(0), prevCount_(0), prevState_(0) {
}

void QuadratureEncoder::begin() {
    // ハードウェア依存: 割り込み設定
    // 実機実装時に追加
}

int32_t QuadratureEncoder::getCount() const {
    return count_;
}

void QuadratureEncoder::resetCount() {
    count_ = 0;
    prevCount_ = 0;
}

float QuadratureEncoder::getRpm(float dt) {
    int32_t currentCount = count_;
    int32_t diff = currentCount - prevCount_;
    prevCount_ = currentCount;
    return calculateRpm(diff, ppr_, dt);
}

float QuadratureEncoder::calculateRpm(int32_t countDiff, uint16_t ppr, float dt) {
    // ゼロ除算回避
    if (dt <= 0.0f || ppr == 0) {
        return 0.0f;
    }

    // RPM = (countDiff / ppr) / dt * 60
    // = countDiff * 60 / (ppr * dt)
    return static_cast<float>(countDiff) * 60.0f / (static_cast<float>(ppr) * dt);
}

int8_t QuadratureEncoder::decodeState(uint8_t prevState, uint8_t currState) {
    // 4逓倍デコードテーブル
    // 状態: (A << 1) | B
    //   0 = 0b00, 1 = 0b01, 2 = 0b10, 3 = 0b11
    //
    // 正転シーケンス: 00→01→11→10→00
    //   0→1→3→2→0 で各遷移 +1
    // 逆転シーケンス: 00→10→11→01→00
    //   0→2→3→1→0 で各遷移 -1
    // 不正遷移（2ステップスキップ）: 0
    //   00↔11, 01↔10
    //
    // テーブル: DECODE_TABLE[prevState][currState] = delta
    static const int8_t DECODE_TABLE[4][4] = {
        //  curr:     0(00)  1(01)  2(10)  3(11)
        /* prev 0(00) */ { 0,    +1,    -1,     0},
        /* prev 1(01) */ {-1,     0,     0,    +1},
        /* prev 2(10) */ {+1,     0,     0,    -1},
        /* prev 3(11) */ { 0,    -1,    +1,     0}
    };

    // 状態は2bitのみ使用
    uint8_t prev = prevState & 0x03;
    uint8_t curr = currState & 0x03;

    return DECODE_TABLE[prev][curr];
}

void QuadratureEncoder::handleInterrupt() {
    // ハードウェア依存: 割り込みハンドラ
    // 実機実装時に追加
}
