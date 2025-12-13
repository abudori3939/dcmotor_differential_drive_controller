#include <unity.h>
#include "PidController.h"

void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// 基本動作テスト
// ============================================================================

void test_pid_p_control_only(void) {
    // Kp=1.0, Ki=0, Kd=0 で誤差100 → 出力100
    PidController pid(1.0f, 0.0f, 0.0f);
    float output = pid.compute(100.0f, 0.0f, 0.01f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, output);
}

void test_pid_zero_error(void) {
    // setpoint == measured → 出力0
    PidController pid(1.0f, 0.0f, 0.0f);
    float output = pid.compute(50.0f, 50.0f, 0.01f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, output);
}

void test_pid_negative_error(void) {
    // setpoint < measured → 負の出力
    PidController pid(1.0f, 0.0f, 0.0f);
    float output = pid.compute(0.0f, 100.0f, 0.01f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -100.0f, output);
}

// ============================================================================
// 積分項（I制御）テスト
// ============================================================================

void test_pid_integral_accumulation(void) {
    // 同じ誤差を複数回computeすると積分が蓄積
    PidController pid(0.0f, 1.0f, 0.0f);  // Ki=1.0のみ
    float dt = 0.01f;
    float error = 100.0f;  // setpoint=100, measured=0

    float output1 = pid.compute(100.0f, 0.0f, dt);
    float output2 = pid.compute(100.0f, 0.0f, dt);
    float output3 = pid.compute(100.0f, 0.0f, dt);

    // 積分値: error * dt * Ki が蓄積
    // output1 = 100 * 0.01 * 1.0 = 1.0
    // output2 = 100 * 0.01 * 2 = 2.0
    // output3 = 100 * 0.01 * 3 = 3.0
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, output1);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f, output2);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, output3);
}

void test_pid_reset_clears_integral(void) {
    // reset()で積分値がクリアされる
    PidController pid(0.0f, 1.0f, 0.0f);

    pid.compute(100.0f, 0.0f, 0.01f);
    pid.compute(100.0f, 0.0f, 0.01f);
    pid.reset();

    float output = pid.compute(100.0f, 0.0f, 0.01f);
    // リセット後なので積分は1回分のみ
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, output);
}

// ============================================================================
// 微分項（D制御）テスト
// ============================================================================

void test_pid_derivative_first_call_zero(void) {
    // 初回呼び出しはD項=0（前回誤差を現在誤差で初期化）
    PidController pid(0.0f, 0.0f, 1.0f);  // Kd=1.0のみ
    float output = pid.compute(100.0f, 0.0f, 0.01f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, output);
}

void test_pid_derivative_no_change(void) {
    // 誤差変化なし → D項=0
    PidController pid(0.0f, 0.0f, 1.0f);
    pid.compute(100.0f, 0.0f, 0.01f);  // 初回
    float output = pid.compute(100.0f, 0.0f, 0.01f);  // 2回目、同じ誤差
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, output);
}

void test_pid_derivative_error_increasing(void) {
    // 誤差が増加 → D項が正
    PidController pid(0.0f, 0.0f, 1.0f);
    float dt = 0.01f;
    pid.compute(50.0f, 0.0f, dt);   // 誤差=50
    float output = pid.compute(100.0f, 0.0f, dt);  // 誤差=100（増加）

    // D項 = Kd * (error - prevError) / dt = 1.0 * (100 - 50) / 0.01 = 5000
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 5000.0f, output);
}

void test_pid_derivative_error_decreasing(void) {
    // 誤差が減少 → D項が負
    PidController pid(0.0f, 0.0f, 1.0f);
    float dt = 0.01f;
    pid.compute(100.0f, 0.0f, dt);  // 誤差=100
    float output = pid.compute(50.0f, 0.0f, dt);   // 誤差=50（減少）

    // D項 = Kd * (error - prevError) / dt = 1.0 * (50 - 100) / 0.01 = -5000
    TEST_ASSERT_FLOAT_WITHIN(0.1f, -5000.0f, output);
}

void test_pid_derivative_after_reset(void) {
    // reset()後もD項=0
    PidController pid(0.0f, 0.0f, 1.0f);
    pid.compute(100.0f, 0.0f, 0.01f);
    pid.compute(50.0f, 0.0f, 0.01f);
    pid.reset();

    float output = pid.compute(100.0f, 0.0f, 0.01f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, output);
}

// ============================================================================
// 出力リミットテスト
// ============================================================================

void test_pid_output_clamp_upper(void) {
    // 上限超過時にクランプ
    PidController pid(10.0f, 0.0f, 0.0f);
    pid.setOutputLimits(-100.0f, 100.0f);
    float output = pid.compute(100.0f, 0.0f, 0.01f);  // 誤差100 × Kp10 = 1000
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, output);
}

void test_pid_output_clamp_lower(void) {
    // 下限超過時にクランプ
    PidController pid(10.0f, 0.0f, 0.0f);
    pid.setOutputLimits(-100.0f, 100.0f);
    float output = pid.compute(0.0f, 100.0f, 0.01f);  // 誤差-100 × Kp10 = -1000
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -100.0f, output);
}

void test_pid_within_limits_unchanged(void) {
    // リミット内なら変更なし
    PidController pid(1.0f, 0.0f, 0.0f);
    pid.setOutputLimits(-100.0f, 100.0f);
    float output = pid.compute(50.0f, 0.0f, 0.01f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, output);
}

// ============================================================================
// アンチワインドアップテスト（条件付き積分）
// ============================================================================

void test_pid_antiwindup_stops_integration_at_upper_saturation(void) {
    // 上限飽和中は積分が蓄積しない
    PidController pid(10.0f, 1.0f, 0.0f);
    pid.setOutputLimits(-100.0f, 100.0f);
    float dt = 0.01f;

    // 誤差100、P項=1000 → 飽和（100にクランプ）
    pid.compute(100.0f, 0.0f, dt);
    pid.compute(100.0f, 0.0f, dt);
    pid.compute(100.0f, 0.0f, dt);

    // 誤差を0にしてP項を消す → 積分項のみ残る
    // アンチワインドアップが効いていれば積分は蓄積されていない
    float output = pid.compute(0.0f, 0.0f, dt);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, output);
}

void test_pid_antiwindup_stops_integration_at_lower_saturation(void) {
    // 下限飽和中も積分が蓄積しない
    PidController pid(10.0f, 1.0f, 0.0f);
    pid.setOutputLimits(-100.0f, 100.0f);
    float dt = 0.01f;

    // 誤差-100、P項=-1000 → 飽和（-100にクランプ）
    pid.compute(0.0f, 100.0f, dt);
    pid.compute(0.0f, 100.0f, dt);
    pid.compute(0.0f, 100.0f, dt);

    // 誤差を0にしてP項を消す
    float output = pid.compute(0.0f, 0.0f, dt);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, output);
}

void test_pid_antiwindup_allows_opposite_integration(void) {
    // 上限飽和中でも誤差が負なら積分を許可（改良版）
    PidController pid(0.0f, 10.0f, 0.0f);  // I制御のみ
    pid.setOutputLimits(-100.0f, 100.0f);
    float dt = 0.1f;

    // 正の誤差で積分を100まで蓄積
    for (int i = 0; i < 20; i++) {
        pid.compute(100.0f, 0.0f, dt);  // 各回 +100（飽和）
    }

    // 負の誤差で積分を減らせることを確認
    float output1 = pid.compute(0.0f, 100.0f, dt);  // 誤差=-100
    float output2 = pid.compute(0.0f, 100.0f, dt);

    // 積分が減少していることを確認
    TEST_ASSERT_TRUE(output2 < output1);
}

// ============================================================================
// dtのガードテスト
// ============================================================================

void test_pid_dt_zero_returns_zero(void) {
    PidController pid(1.0f, 1.0f, 1.0f);
    float output = pid.compute(100.0f, 0.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, output);
}

void test_pid_dt_negative_returns_zero(void) {
    PidController pid(1.0f, 1.0f, 1.0f);
    float output = pid.compute(100.0f, 0.0f, -0.01f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, output);
}

// ============================================================================
// setGainsテスト
// ============================================================================

void test_pid_set_gains(void) {
    PidController pid(1.0f, 0.0f, 0.0f);
    pid.setGains(2.0f, 0.0f, 0.0f);
    float output = pid.compute(50.0f, 0.0f, 0.01f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, output);  // 50 * 2.0 = 100
}

// ============================================================================
// メイン
// ============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // 基本動作テスト
    RUN_TEST(test_pid_p_control_only);
    RUN_TEST(test_pid_zero_error);
    RUN_TEST(test_pid_negative_error);

    // 積分項テスト
    RUN_TEST(test_pid_integral_accumulation);
    RUN_TEST(test_pid_reset_clears_integral);

    // 微分項テスト
    RUN_TEST(test_pid_derivative_first_call_zero);
    RUN_TEST(test_pid_derivative_no_change);
    RUN_TEST(test_pid_derivative_error_increasing);
    RUN_TEST(test_pid_derivative_error_decreasing);
    RUN_TEST(test_pid_derivative_after_reset);

    // 出力リミットテスト
    RUN_TEST(test_pid_output_clamp_upper);
    RUN_TEST(test_pid_output_clamp_lower);
    RUN_TEST(test_pid_within_limits_unchanged);

    // アンチワインドアップテスト
    RUN_TEST(test_pid_antiwindup_stops_integration_at_upper_saturation);
    RUN_TEST(test_pid_antiwindup_stops_integration_at_lower_saturation);
    RUN_TEST(test_pid_antiwindup_allows_opposite_integration);

    // dtガードテスト
    RUN_TEST(test_pid_dt_zero_returns_zero);
    RUN_TEST(test_pid_dt_negative_returns_zero);

    // setGainsテスト
    RUN_TEST(test_pid_set_gains);

    return UNITY_END();
}
