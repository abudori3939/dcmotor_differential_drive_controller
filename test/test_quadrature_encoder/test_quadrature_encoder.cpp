/**
 * QuadratureEncoder ユニットテスト
 *
 * 1. RPM計算ロジック（カウント差→RPM）
 * 2. 4逓倍デコードロジック（A/B相状態遷移→カウント増減）
 */

#include <unity.h>
#include "QuadratureEncoder.h"

void setUp(void) {}
void tearDown(void) {}

// ============================================================
// RPM計算テスト
// 計算式: RPM = (countDiff / ppr) / dt * 60
// ============================================================

// 正転: 1秒間に1回転 = 60 RPM
void test_rpm_forward_one_rotation(void) {
    // PPR=1024, 1秒間に1024カウント = 1回転/秒 = 60RPM
    float rpm = QuadratureEncoder::calculateRpm(1024, 1024, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 60.0f, rpm);
}

// 逆転: -60 RPM
void test_rpm_reverse_one_rotation(void) {
    float rpm = QuadratureEncoder::calculateRpm(-1024, 1024, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, -60.0f, rpm);
}

// 停止: 0 RPM
void test_rpm_stopped(void) {
    float rpm = QuadratureEncoder::calculateRpm(0, 1024, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, rpm);
}

// 高速回転: 2回転/0.5秒 = 240 RPM
void test_rpm_high_speed(void) {
    // 2048カウント / 0.5秒 = 4回転/秒 = 240RPM
    float rpm = QuadratureEncoder::calculateRpm(2048, 1024, 0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 240.0f, rpm);
}

// PPR=512の場合
void test_rpm_ppr_512(void) {
    float rpm = QuadratureEncoder::calculateRpm(512, 512, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 60.0f, rpm);
}

// PPR=2048の場合
void test_rpm_ppr_2048(void) {
    float rpm = QuadratureEncoder::calculateRpm(2048, 2048, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 60.0f, rpm);
}

// dt=0.01秒（100Hz制御ループ）
void test_rpm_short_dt(void) {
    // 10カウント / 0.01秒, PPR=1024 = 58.59RPM
    float rpm = QuadratureEncoder::calculateRpm(10, 1024, 0.01f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 58.59f, rpm);
}

// dt=0の場合: 0を返す（ゼロ除算回避）
void test_rpm_zero_dt(void) {
    float rpm = QuadratureEncoder::calculateRpm(1024, 1024, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, rpm);
}

// dt<0の場合: 0を返す（不正な入力）
void test_rpm_negative_dt(void) {
    float rpm = QuadratureEncoder::calculateRpm(1024, 1024, -0.01f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, rpm);
}

// ppr=0の場合: 0を返す（ゼロ除算回避）
void test_rpm_zero_ppr(void) {
    float rpm = QuadratureEncoder::calculateRpm(1024, 0, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, rpm);
}

// ============================================================
// 4逓倍デコードロジックテスト
// 状態: (A << 1) | B で 0b00, 0b01, 0b10, 0b11
// 正転シーケンス: 00→01→11→10→00 (Gray code)
// 逆転シーケンス: 00→10→11→01→00
// ============================================================

// --- 正転テスト（各遷移で+1） ---

void test_decode_forward_00_to_01(void) {
    int8_t delta = QuadratureEncoder::decodeState(0b00, 0b01);
    TEST_ASSERT_EQUAL_INT8(1, delta);
}

void test_decode_forward_01_to_11(void) {
    int8_t delta = QuadratureEncoder::decodeState(0b01, 0b11);
    TEST_ASSERT_EQUAL_INT8(1, delta);
}

void test_decode_forward_11_to_10(void) {
    int8_t delta = QuadratureEncoder::decodeState(0b11, 0b10);
    TEST_ASSERT_EQUAL_INT8(1, delta);
}

void test_decode_forward_10_to_00(void) {
    int8_t delta = QuadratureEncoder::decodeState(0b10, 0b00);
    TEST_ASSERT_EQUAL_INT8(1, delta);
}

// --- 逆転テスト（各遷移で-1） ---

void test_decode_reverse_00_to_10(void) {
    int8_t delta = QuadratureEncoder::decodeState(0b00, 0b10);
    TEST_ASSERT_EQUAL_INT8(-1, delta);
}

void test_decode_reverse_10_to_11(void) {
    int8_t delta = QuadratureEncoder::decodeState(0b10, 0b11);
    TEST_ASSERT_EQUAL_INT8(-1, delta);
}

void test_decode_reverse_11_to_01(void) {
    int8_t delta = QuadratureEncoder::decodeState(0b11, 0b01);
    TEST_ASSERT_EQUAL_INT8(-1, delta);
}

void test_decode_reverse_01_to_00(void) {
    int8_t delta = QuadratureEncoder::decodeState(0b01, 0b00);
    TEST_ASSERT_EQUAL_INT8(-1, delta);
}

// --- 状態変化なし（カウント変化0） ---

void test_decode_no_change_00(void) {
    int8_t delta = QuadratureEncoder::decodeState(0b00, 0b00);
    TEST_ASSERT_EQUAL_INT8(0, delta);
}

void test_decode_no_change_01(void) {
    int8_t delta = QuadratureEncoder::decodeState(0b01, 0b01);
    TEST_ASSERT_EQUAL_INT8(0, delta);
}

void test_decode_no_change_10(void) {
    int8_t delta = QuadratureEncoder::decodeState(0b10, 0b10);
    TEST_ASSERT_EQUAL_INT8(0, delta);
}

void test_decode_no_change_11(void) {
    int8_t delta = QuadratureEncoder::decodeState(0b11, 0b11);
    TEST_ASSERT_EQUAL_INT8(0, delta);
}

// --- 不正遷移（2ステップスキップ = ノイズ）→ 0を返す ---

void test_decode_invalid_00_to_11(void) {
    int8_t delta = QuadratureEncoder::decodeState(0b00, 0b11);
    TEST_ASSERT_EQUAL_INT8(0, delta);
}

void test_decode_invalid_01_to_10(void) {
    int8_t delta = QuadratureEncoder::decodeState(0b01, 0b10);
    TEST_ASSERT_EQUAL_INT8(0, delta);
}

void test_decode_invalid_10_to_01(void) {
    int8_t delta = QuadratureEncoder::decodeState(0b10, 0b01);
    TEST_ASSERT_EQUAL_INT8(0, delta);
}

void test_decode_invalid_11_to_00(void) {
    int8_t delta = QuadratureEncoder::decodeState(0b11, 0b00);
    TEST_ASSERT_EQUAL_INT8(0, delta);
}

// --- 完全シーケンステスト ---

// 1サイクル正転（4カウント増加）
void test_decode_full_forward_cycle(void) {
    int32_t count = 0;
    count += QuadratureEncoder::decodeState(0b00, 0b01);  // +1
    count += QuadratureEncoder::decodeState(0b01, 0b11);  // +1
    count += QuadratureEncoder::decodeState(0b11, 0b10);  // +1
    count += QuadratureEncoder::decodeState(0b10, 0b00);  // +1
    TEST_ASSERT_EQUAL_INT32(4, count);
}

// 1サイクル逆転（4カウント減少）
void test_decode_full_reverse_cycle(void) {
    int32_t count = 0;
    count += QuadratureEncoder::decodeState(0b00, 0b10);  // -1
    count += QuadratureEncoder::decodeState(0b10, 0b11);  // -1
    count += QuadratureEncoder::decodeState(0b11, 0b01);  // -1
    count += QuadratureEncoder::decodeState(0b01, 0b00);  // -1
    TEST_ASSERT_EQUAL_INT32(-4, count);
}

// 往復（正転2 + 逆転2 = 0）
void test_decode_forward_then_reverse(void) {
    int32_t count = 0;
    // 正転2ステップ
    count += QuadratureEncoder::decodeState(0b00, 0b01);  // +1
    count += QuadratureEncoder::decodeState(0b01, 0b11);  // +1
    // 逆転2ステップ
    count += QuadratureEncoder::decodeState(0b11, 0b01);  // -1
    count += QuadratureEncoder::decodeState(0b01, 0b00);  // -1
    TEST_ASSERT_EQUAL_INT32(0, count);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // RPM計算テスト
    RUN_TEST(test_rpm_forward_one_rotation);
    RUN_TEST(test_rpm_reverse_one_rotation);
    RUN_TEST(test_rpm_stopped);
    RUN_TEST(test_rpm_high_speed);
    RUN_TEST(test_rpm_ppr_512);
    RUN_TEST(test_rpm_ppr_2048);
    RUN_TEST(test_rpm_short_dt);
    RUN_TEST(test_rpm_zero_dt);
    RUN_TEST(test_rpm_negative_dt);
    RUN_TEST(test_rpm_zero_ppr);

    // 4逓倍デコード: 正転
    RUN_TEST(test_decode_forward_00_to_01);
    RUN_TEST(test_decode_forward_01_to_11);
    RUN_TEST(test_decode_forward_11_to_10);
    RUN_TEST(test_decode_forward_10_to_00);

    // 4逓倍デコード: 逆転
    RUN_TEST(test_decode_reverse_00_to_10);
    RUN_TEST(test_decode_reverse_10_to_11);
    RUN_TEST(test_decode_reverse_11_to_01);
    RUN_TEST(test_decode_reverse_01_to_00);

    // 4逓倍デコード: 状態変化なし
    RUN_TEST(test_decode_no_change_00);
    RUN_TEST(test_decode_no_change_01);
    RUN_TEST(test_decode_no_change_10);
    RUN_TEST(test_decode_no_change_11);

    // 4逓倍デコード: 不正遷移
    RUN_TEST(test_decode_invalid_00_to_11);
    RUN_TEST(test_decode_invalid_01_to_10);
    RUN_TEST(test_decode_invalid_10_to_01);
    RUN_TEST(test_decode_invalid_11_to_00);

    // 4逓倍デコード: シーケンス
    RUN_TEST(test_decode_full_forward_cycle);
    RUN_TEST(test_decode_full_reverse_cycle);
    RUN_TEST(test_decode_forward_then_reverse);

    return UNITY_END();
}
