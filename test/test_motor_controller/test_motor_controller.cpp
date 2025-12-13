/**
 * @file test_motor_controller.cpp
 * @brief MotorController ユニットテスト
 *
 * ハードウェア非依存のロジック部分のみテスト
 * - setCmdVel()で目標RPMが正しく計算されること
 * - 回転優先クランプが正しく動作すること
 */

#include <unity.h>
#include "MotorController.h"

// テスト用のロボットパラメータ
static const float WHEEL_DIAMETER = 0.1f;  // 100mm
static const float TRACK_WIDTH = 0.3f;     // 300mm
static const float GEAR_RATIO = 1.0f;
static const float MAX_RPM = 200.0f;

void setUp(void) {
}

void tearDown(void) {
}

// =============================================================================
// 目標RPM計算テスト（setCmdVel + getTargetRpm）
// =============================================================================

/**
 * @test 前進のみ
 */
void test_setCmdVel_forward(void) {
    MotorController controller(WHEEL_DIAMETER, TRACK_WIDTH, GEAR_RATIO, MAX_RPM);

    controller.setCmdVel(0.1f, 0.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, 19.1f, controller.getTargetRpmL());
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 19.1f, controller.getTargetRpmR());
}

/**
 * @test 後退のみ
 */
void test_setCmdVel_backward(void) {
    MotorController controller(WHEEL_DIAMETER, TRACK_WIDTH, GEAR_RATIO, MAX_RPM);

    controller.setCmdVel(-0.1f, 0.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, -19.1f, controller.getTargetRpmL());
    TEST_ASSERT_FLOAT_WITHIN(0.1f, -19.1f, controller.getTargetRpmR());
}

/**
 * @test 左旋回（その場）
 */
void test_setCmdVel_rotate_left(void) {
    MotorController controller(WHEEL_DIAMETER, TRACK_WIDTH, GEAR_RATIO, MAX_RPM);

    controller.setCmdVel(0.0f, 1.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, -28.6f, controller.getTargetRpmL());
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 28.6f, controller.getTargetRpmR());
}

/**
 * @test 右旋回（その場）
 */
void test_setCmdVel_rotate_right(void) {
    MotorController controller(WHEEL_DIAMETER, TRACK_WIDTH, GEAR_RATIO, MAX_RPM);

    controller.setCmdVel(0.0f, -1.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, 28.6f, controller.getTargetRpmL());
    TEST_ASSERT_FLOAT_WITHIN(0.1f, -28.6f, controller.getTargetRpmR());
}

/**
 * @test 停止
 */
void test_setCmdVel_stop(void) {
    MotorController controller(WHEEL_DIAMETER, TRACK_WIDTH, GEAR_RATIO, MAX_RPM);

    controller.setCmdVel(0.0f, 0.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, controller.getTargetRpmL());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, controller.getTargetRpmR());
}

// =============================================================================
// 回転優先クランプテスト
// =============================================================================

/**
 * @test 上限以内はクランプされない
 */
void test_clamp_within_limit(void) {
    MotorController controller(WHEEL_DIAMETER, TRACK_WIDTH, GEAR_RATIO, 100.0f);

    // 50 RPM程度を生成するcmd_vel
    controller.setCmdVel(0.262f, 0.0f);

    // クランプされず50 RPM付近のまま
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 50.0f, controller.getTargetRpmL());
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 50.0f, controller.getTargetRpmR());
}

/**
 * @test 直進のみで上限超過時は両輪クランプ
 * 回転成分がないので単純クランプと同じ結果
 */
void test_clamp_straight_over_limit(void) {
    MotorController controller(WHEEL_DIAMETER, TRACK_WIDTH, GEAR_RATIO, 100.0f);

    // 150 RPM以上を生成するcmd_vel
    controller.setCmdVel(0.8f, 0.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, controller.getTargetRpmL());
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, controller.getTargetRpmR());
}

/**
 * @test 後退で上限超過時は両輪クランプ
 */
void test_clamp_reverse_over_limit(void) {
    MotorController controller(WHEEL_DIAMETER, TRACK_WIDTH, GEAR_RATIO, 100.0f);

    controller.setCmdVel(-0.8f, 0.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, -100.0f, controller.getTargetRpmL());
    TEST_ASSERT_FLOAT_WITHIN(0.1f, -100.0f, controller.getTargetRpmR());
}

/**
 * @test 旋回しながら前進時、回転を維持して直進を減速
 *
 * cmd_vel で左=70, 右=130 相当を生成し、max_rpm=100でクランプ
 * 回転成分 = (130-70)/2 = 30 は維持
 * 直進成分 = (130+70)/2 = 100 → 100-30=70 にクランプ
 * 結果: 左=70-30=40, 右=70+30=100
 */
void test_clamp_rotation_priority_preserves_rotation(void) {
    // 左=70, 右=130のRPMを生成するcmd_velを計算
    // left_vel = 70 * 2*PI*0.05 / 60 = 0.3665 m/s
    // right_vel = 130 * 2*PI*0.05 / 60 = 0.6807 m/s
    // linear_x = (0.3665 + 0.6807) / 2 = 0.5236 m/s
    // angular_z = (0.6807 - 0.3665) / 0.3 = 1.047 rad/s
    MotorController controller(WHEEL_DIAMETER, TRACK_WIDTH, GEAR_RATIO, 100.0f);

    controller.setCmdVel(0.5236f, 1.047f);

    // 回転優先クランプ後:
    // v_rot = 30 (維持、100以内)
    // v_trans_limit = 100 - 30 = 70
    // v_trans = 100 → 70 にクランプ
    // left = 70 - 30 = 40, right = 70 + 30 = 100
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 40.0f, controller.getTargetRpmL());
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, controller.getTargetRpmR());
}

/**
 * @test 両輪とも上限超過の場合、回転を維持しつつ両輪を制限内に
 */
void test_clamp_both_wheels_exceed(void) {
    MotorController controller(WHEEL_DIAMETER, TRACK_WIDTH, GEAR_RATIO, 100.0f);

    // 非常に高速なcmd_vel
    controller.setCmdVel(1.0f, 1.0f);

    // 結果が最大RPM以内であること
    TEST_ASSERT_TRUE(controller.getTargetRpmL() >= -100.0f - 0.1f);
    TEST_ASSERT_TRUE(controller.getTargetRpmL() <= 100.0f + 0.1f);
    TEST_ASSERT_TRUE(controller.getTargetRpmR() >= -100.0f - 0.1f);
    TEST_ASSERT_TRUE(controller.getTargetRpmR() <= 100.0f + 0.1f);
}

/**
 * @test その場旋回は上限以内なら変更なし
 */
void test_clamp_pure_rotation_within_limit(void) {
    MotorController controller(WHEEL_DIAMETER, TRACK_WIDTH, GEAR_RATIO, 100.0f);

    // angular_z=1.0 → 左=-28.6, 右=+28.6（上限内）
    controller.setCmdVel(0.0f, 1.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, -28.6f, controller.getTargetRpmL());
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 28.6f, controller.getTargetRpmR());
}

/**
 * @test その場旋回が上限超過時はクランプ
 */
void test_clamp_pure_rotation_over_limit(void) {
    MotorController controller(WHEEL_DIAMETER, TRACK_WIDTH, GEAR_RATIO, 50.0f);

    // angular_z=2.0 → 左=-57.3, 右=+57.3（上限超過）
    controller.setCmdVel(0.0f, 2.0f);

    // 直進成分0なので、回転成分のみクランプ
    TEST_ASSERT_FLOAT_WITHIN(0.1f, -50.0f, controller.getTargetRpmL());
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 50.0f, controller.getTargetRpmR());
}

// =============================================================================
// 初期状態テスト
// =============================================================================

/**
 * @test 初期状態では目標RPMは0
 */
void test_initial_target_rpm_zero(void) {
    MotorController controller(WHEEL_DIAMETER, TRACK_WIDTH, GEAR_RATIO, MAX_RPM);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, controller.getTargetRpmL());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, controller.getTargetRpmR());
}

// =============================================================================
// メイン
// =============================================================================

int main(void) {
    UNITY_BEGIN();

    // 目標RPM計算テスト
    RUN_TEST(test_setCmdVel_forward);
    RUN_TEST(test_setCmdVel_backward);
    RUN_TEST(test_setCmdVel_rotate_left);
    RUN_TEST(test_setCmdVel_rotate_right);
    RUN_TEST(test_setCmdVel_stop);

    // 回転優先クランプテスト
    RUN_TEST(test_clamp_within_limit);
    RUN_TEST(test_clamp_straight_over_limit);
    RUN_TEST(test_clamp_reverse_over_limit);
    RUN_TEST(test_clamp_rotation_priority_preserves_rotation);
    RUN_TEST(test_clamp_both_wheels_exceed);
    RUN_TEST(test_clamp_pure_rotation_within_limit);
    RUN_TEST(test_clamp_pure_rotation_over_limit);

    // 初期状態テスト
    RUN_TEST(test_initial_target_rpm_zero);

    return UNITY_END();
}
