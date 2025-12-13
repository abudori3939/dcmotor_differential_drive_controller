/**
 * @file test_differential_kinematics.cpp
 * @brief DifferentialKinematics ユニットテスト
 *
 * cmd_vel（linear_x, angular_z）から左右ホイールRPMへの変換テスト
 *
 * テスト条件:
 * - wheel_diameter = 0.1m
 * - track_width = 0.3m
 * - gear_ratio = 1.0
 */

#include <unity.h>
#include "DifferentialKinematics.h"

// テスト用のロボットパラメータ
static const float WHEEL_DIAMETER = 0.1f;  // 100mm
static const float TRACK_WIDTH = 0.3f;     // 300mm
static const float GEAR_RATIO = 1.0f;

static DifferentialKinematics kinematics(WHEEL_DIAMETER, TRACK_WIDTH, GEAR_RATIO);

void setUp(void) {
    // 各テスト前の初期化
}

void tearDown(void) {
    // 各テスト後のクリーンアップ
}

// =============================================================================
// 基本動作テスト
// =============================================================================

/**
 * @test 前進のみ
 * linear_x=0.1 m/s, angular_z=0 → 左右同じRPM
 */
void test_forward_only(void) {
    float leftRpm, rightRpm;
    kinematics.calculate(0.1f, 0.0f, leftRpm, rightRpm);

    // 0.1 / (2 * PI * 0.05) * 60 = 19.0986...
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 19.1f, leftRpm);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 19.1f, rightRpm);
}

/**
 * @test 後退のみ
 * linear_x=-0.1 m/s, angular_z=0 → 左右同じ負のRPM
 */
void test_backward_only(void) {
    float leftRpm, rightRpm;
    kinematics.calculate(-0.1f, 0.0f, leftRpm, rightRpm);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, -19.1f, leftRpm);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, -19.1f, rightRpm);
}

/**
 * @test 左旋回（その場）
 * linear_x=0, angular_z=1.0 rad/s → 左が負、右が正
 */
void test_rotate_left_in_place(void) {
    float leftRpm, rightRpm;
    kinematics.calculate(0.0f, 1.0f, leftRpm, rightRpm);

    // left_vel = 0 - 1.0 * 0.15 = -0.15 m/s → -28.6 RPM
    // right_vel = 0 + 1.0 * 0.15 = 0.15 m/s → 28.6 RPM
    TEST_ASSERT_FLOAT_WITHIN(0.1f, -28.6f, leftRpm);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 28.6f, rightRpm);
}

/**
 * @test 右旋回（その場）
 * linear_x=0, angular_z=-1.0 rad/s → 左が正、右が負
 */
void test_rotate_right_in_place(void) {
    float leftRpm, rightRpm;
    kinematics.calculate(0.0f, -1.0f, leftRpm, rightRpm);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, 28.6f, leftRpm);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, -28.6f, rightRpm);
}

/**
 * @test 前進 + 左旋回
 * linear_x=0.1 m/s, angular_z=0.5 rad/s
 */
void test_forward_with_left_turn(void) {
    float leftRpm, rightRpm;
    kinematics.calculate(0.1f, 0.5f, leftRpm, rightRpm);

    // left_vel = 0.1 - 0.5 * 0.15 = 0.025 m/s → 4.77 RPM
    // right_vel = 0.1 + 0.5 * 0.15 = 0.175 m/s → 33.42 RPM
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 4.8f, leftRpm);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 33.4f, rightRpm);
}

/**
 * @test 前進 + 右旋回
 * linear_x=0.1 m/s, angular_z=-0.5 rad/s
 */
void test_forward_with_right_turn(void) {
    float leftRpm, rightRpm;
    kinematics.calculate(0.1f, -0.5f, leftRpm, rightRpm);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, 33.4f, leftRpm);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 4.8f, rightRpm);
}

/**
 * @test 停止
 * linear_x=0, angular_z=0 → 両方0
 */
void test_stop(void) {
    float leftRpm, rightRpm;
    kinematics.calculate(0.0f, 0.0f, leftRpm, rightRpm);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, leftRpm);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, rightRpm);
}

// =============================================================================
// パラメータ違いテスト
// =============================================================================

/**
 * @test 減速比2.0の場合
 * ギア比が2倍ならRPMも2倍
 */
void test_gear_ratio_effect(void) {
    DifferentialKinematics kinematicsWithGear(WHEEL_DIAMETER, TRACK_WIDTH, 2.0f);

    float leftRpm, rightRpm;
    kinematicsWithGear.calculate(0.1f, 0.0f, leftRpm, rightRpm);

    // 基本RPM 19.1 × gear_ratio 2.0 = 38.2
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 38.2f, leftRpm);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 38.2f, rightRpm);
}

/**
 * @test ホイール直径が大きい場合
 * 直径が2倍ならRPMは半分
 */
void test_larger_wheel_diameter(void) {
    DifferentialKinematics kinematicsLargeWheel(0.2f, TRACK_WIDTH, GEAR_RATIO);

    float leftRpm, rightRpm;
    kinematicsLargeWheel.calculate(0.1f, 0.0f, leftRpm, rightRpm);

    // 直径2倍 → RPM半分: 19.1 / 2 = 9.55
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 9.55f, leftRpm);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 9.55f, rightRpm);
}

/**
 * @test トレッド幅が広い場合
 * 同じ角速度でも旋回時の左右差が大きくなる
 */
void test_wider_track_width(void) {
    DifferentialKinematics kinematicsWideTrack(WHEEL_DIAMETER, 0.6f, GEAR_RATIO);

    float leftRpm, rightRpm;
    kinematicsWideTrack.calculate(0.0f, 1.0f, leftRpm, rightRpm);

    // left_vel = 0 - 1.0 * 0.3 = -0.3 m/s → -57.3 RPM
    // right_vel = 0 + 1.0 * 0.3 = 0.3 m/s → 57.3 RPM
    TEST_ASSERT_FLOAT_WITHIN(0.1f, -57.3f, leftRpm);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 57.3f, rightRpm);
}

// =============================================================================
// メイン
// =============================================================================

int main(void) {
    UNITY_BEGIN();

    // 基本動作テスト
    RUN_TEST(test_forward_only);
    RUN_TEST(test_backward_only);
    RUN_TEST(test_rotate_left_in_place);
    RUN_TEST(test_rotate_right_in_place);
    RUN_TEST(test_forward_with_left_turn);
    RUN_TEST(test_forward_with_right_turn);
    RUN_TEST(test_stop);

    // パラメータ違いテスト
    RUN_TEST(test_gear_ratio_effect);
    RUN_TEST(test_larger_wheel_diameter);
    RUN_TEST(test_wider_track_width);

    return UNITY_END();
}
