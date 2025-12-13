#include <unity.h>
#include "SharedMotorData.h"

void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// CmdVelData 初期化テスト
// ============================================================================

void test_cmd_vel_data_init_linear_x(void) {
    // 初期化後、linearXは0
    volatile CmdVelData data;
    data.linearX = 999.0f;  // 事前に値を設定
    initCmdVelData(&data);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, data.linearX);
}

void test_cmd_vel_data_init_angular_z(void) {
    // 初期化後、angularZは0
    volatile CmdVelData data;
    data.angularZ = 999.0f;
    initCmdVelData(&data);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, data.angularZ);
}

void test_cmd_vel_data_init_failsafe_stop(void) {
    // 初期化後、failsafeStopはfalse
    volatile CmdVelData data;
    data.failsafeStop = true;
    initCmdVelData(&data);
    TEST_ASSERT_FALSE(data.failsafeStop);
}

// ============================================================================
// MotorStateData 初期化テスト
// ============================================================================

void test_motor_state_data_init_encoder_count_l(void) {
    // 初期化後、encoderCountLは0
    volatile MotorStateData data;
    data.encoderCountL = 12345;
    initMotorStateData(&data);
    TEST_ASSERT_EQUAL_INT32(0, data.encoderCountL);
}

void test_motor_state_data_init_encoder_count_r(void) {
    // 初期化後、encoderCountRは0
    volatile MotorStateData data;
    data.encoderCountR = 12345;
    initMotorStateData(&data);
    TEST_ASSERT_EQUAL_INT32(0, data.encoderCountR);
}

void test_motor_state_data_init_target_rpm_l(void) {
    // 初期化後、targetRpmLは0
    volatile MotorStateData data;
    data.targetRpmL = 999.0f;
    initMotorStateData(&data);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, data.targetRpmL);
}

void test_motor_state_data_init_target_rpm_r(void) {
    // 初期化後、targetRpmRは0
    volatile MotorStateData data;
    data.targetRpmR = 999.0f;
    initMotorStateData(&data);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, data.targetRpmR);
}

void test_motor_state_data_init_current_rpm_l(void) {
    // 初期化後、currentRpmLは0
    volatile MotorStateData data;
    data.currentRpmL = 999.0f;
    initMotorStateData(&data);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, data.currentRpmL);
}

void test_motor_state_data_init_current_rpm_r(void) {
    // 初期化後、currentRpmRは0
    volatile MotorStateData data;
    data.currentRpmR = 999.0f;
    initMotorStateData(&data);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, data.currentRpmR);
}

// ============================================================================
// データ読み書きテスト
// ============================================================================

void test_cmd_vel_data_read_write(void) {
    // 値を設定して正しく読み込めるか
    volatile CmdVelData data;
    initCmdVelData(&data);

    data.linearX = 1.5f;
    data.angularZ = -0.5f;
    data.failsafeStop = true;

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, data.linearX);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.5f, data.angularZ);
    TEST_ASSERT_TRUE(data.failsafeStop);
}

void test_motor_state_data_read_write(void) {
    // 値を設定して正しく読み込めるか
    volatile MotorStateData data;
    initMotorStateData(&data);

    data.encoderCountL = 1000;
    data.encoderCountR = -2000;
    data.targetRpmL = 100.0f;
    data.targetRpmR = 150.0f;
    data.currentRpmL = 98.5f;
    data.currentRpmR = 148.2f;

    TEST_ASSERT_EQUAL_INT32(1000, data.encoderCountL);
    TEST_ASSERT_EQUAL_INT32(-2000, data.encoderCountR);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, data.targetRpmL);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 150.0f, data.targetRpmR);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 98.5f, data.currentRpmL);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 148.2f, data.currentRpmR);
}

// ============================================================================
// メイン
// ============================================================================

int main(void) {
    UNITY_BEGIN();

    // CmdVelData初期化テスト
    RUN_TEST(test_cmd_vel_data_init_linear_x);
    RUN_TEST(test_cmd_vel_data_init_angular_z);
    RUN_TEST(test_cmd_vel_data_init_failsafe_stop);

    // MotorStateData初期化テスト
    RUN_TEST(test_motor_state_data_init_encoder_count_l);
    RUN_TEST(test_motor_state_data_init_encoder_count_r);
    RUN_TEST(test_motor_state_data_init_target_rpm_l);
    RUN_TEST(test_motor_state_data_init_target_rpm_r);
    RUN_TEST(test_motor_state_data_init_current_rpm_l);
    RUN_TEST(test_motor_state_data_init_current_rpm_r);

    // データ読み書きテスト
    RUN_TEST(test_cmd_vel_data_read_write);
    RUN_TEST(test_motor_state_data_read_write);

    return UNITY_END();
}
