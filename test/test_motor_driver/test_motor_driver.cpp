/**
 * MotorDriver テスト
 *
 * ハードウェア非依存のロジック部分をテスト:
 * - 速度値のクランプ（-1.0〜1.0）
 * - PWMデューティサイクル計算
 * - 方向判定
 */

#include <unity.h>
#include "MotorDriver.h"

void setUp(void) {}
void tearDown(void) {}

// =============================================================================
// 速度クランプテスト
// =============================================================================

void test_clampSpeed_within_range(void) {
    // 範囲内の値はそのまま
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, MotorDriver::clampSpeed(0.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.5f, MotorDriver::clampSpeed(-0.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, MotorDriver::clampSpeed(0.0f));
}

void test_clampSpeed_upper_limit(void) {
    // 1.0を超える値は1.0にクランプ
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, MotorDriver::clampSpeed(1.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, MotorDriver::clampSpeed(100.0f));
}

void test_clampSpeed_lower_limit(void) {
    // -1.0未満の値は-1.0にクランプ
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, MotorDriver::clampSpeed(-1.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, MotorDriver::clampSpeed(-100.0f));
}

void test_clampSpeed_boundary_values(void) {
    // 境界値
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, MotorDriver::clampSpeed(1.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, MotorDriver::clampSpeed(-1.0f));
}

// =============================================================================
// 方向判定テスト
// モータドライバの仕様: DIR=LOW(false)で正転、DIR=HIGH(true)で逆転
// =============================================================================

void test_getDirection_positive_speed(void) {
    // 正の速度は正転（DIR = LOW = false）
    TEST_ASSERT_FALSE(MotorDriver::getDirection(0.5f));
    TEST_ASSERT_FALSE(MotorDriver::getDirection(1.0f));
    TEST_ASSERT_FALSE(MotorDriver::getDirection(0.001f));
}

void test_getDirection_negative_speed(void) {
    // 負の速度は逆転（DIR = HIGH = true）
    TEST_ASSERT_TRUE(MotorDriver::getDirection(-0.5f));
    TEST_ASSERT_TRUE(MotorDriver::getDirection(-1.0f));
    TEST_ASSERT_TRUE(MotorDriver::getDirection(-0.001f));
}

void test_getDirection_zero_speed(void) {
    // 速度0は正転方向（DIR = LOW = false）
    TEST_ASSERT_FALSE(MotorDriver::getDirection(0.0f));
}

// =============================================================================
// PWMデューティサイクル計算テスト
// =============================================================================

void test_calculatePwmDuty_full_speed(void) {
    // 最大速度（±1.0）でPWM_MAX
    TEST_ASSERT_EQUAL_UINT8(255, MotorDriver::calculatePwmDuty(1.0f));
    TEST_ASSERT_EQUAL_UINT8(255, MotorDriver::calculatePwmDuty(-1.0f));
}

void test_calculatePwmDuty_zero_speed(void) {
    // 速度0でPWM 0
    TEST_ASSERT_EQUAL_UINT8(0, MotorDriver::calculatePwmDuty(0.0f));
}

void test_calculatePwmDuty_half_speed(void) {
    // 半分の速度でPWM約127-128
    uint8_t duty = MotorDriver::calculatePwmDuty(0.5f);
    TEST_ASSERT_TRUE(duty >= 127 && duty <= 128);

    // 負の値でも絶対値が使われる
    duty = MotorDriver::calculatePwmDuty(-0.5f);
    TEST_ASSERT_TRUE(duty >= 127 && duty <= 128);
}

void test_calculatePwmDuty_quarter_speed(void) {
    // 1/4の速度でPWM約63-64
    uint8_t duty = MotorDriver::calculatePwmDuty(0.25f);
    TEST_ASSERT_TRUE(duty >= 63 && duty <= 64);
}

// =============================================================================
// メイン
// =============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // 速度クランプテスト
    RUN_TEST(test_clampSpeed_within_range);
    RUN_TEST(test_clampSpeed_upper_limit);
    RUN_TEST(test_clampSpeed_lower_limit);
    RUN_TEST(test_clampSpeed_boundary_values);

    // 方向判定テスト
    RUN_TEST(test_getDirection_positive_speed);
    RUN_TEST(test_getDirection_negative_speed);
    RUN_TEST(test_getDirection_zero_speed);

    // PWMデューティサイクル計算テスト
    RUN_TEST(test_calculatePwmDuty_full_speed);
    RUN_TEST(test_calculatePwmDuty_zero_speed);
    RUN_TEST(test_calculatePwmDuty_half_speed);
    RUN_TEST(test_calculatePwmDuty_quarter_speed);

    return UNITY_END();
}
