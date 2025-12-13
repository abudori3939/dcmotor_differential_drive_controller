# 実装進捗

汎用DCモータ差動二輪コントローラへのリファクタリング進捗。

## ステータス凡例

- ⬜ 未着手
- 🔲 テスト作成中
- 🟨 実装中
- ✅ 完了

## 実装タスク

### Phase 1: 基盤整備・設計

| タスク | ステータス | 備考 |
|--------|----------|------|
| architecture.md作成 | ✅ | デュアルコア構成、ライブラリ設計 |
| protocol.md作成 | ✅ | リクエスト・レスポンス型通信仕様 |
| test_specifications.md作成 | ✅ | TDDテストケース仕様 |
| development.md更新 | ✅ | 新構成に合わせて更新 |
| HardwareConfig作成 | ✅ | ピンアサイン定義 |

### Phase 2: PIDController（TDD）

| タスク | ステータス | 備考 |
|--------|----------|------|
| テスト作成 | ✅ | 19テストケース作成 |
| PIDController実装 | ✅ | アンチワインドアップ（条件付き積分）対応 |
| テストパス確認 | ✅ | 全42テストパス |

### Phase 3: エンコーダ・モータドライバ

| タスク | ステータス | 備考 |
|--------|----------|------|
| QuadratureEncoder RPM計算テスト | ✅ | 10テストケース |
| QuadratureEncoder 4逓倍デコードテスト | ✅ | 19テストケース |
| QuadratureEncoder 反転フラグテスト | ✅ | 13テストケース（差動二輪対応） |
| QuadratureEncoder実装 | 🟨 | ロジック実装済、割り込み処理は実機で |
| MotorDriverテスト | ✅ | 15テストケース（速度クランプ、方向判定、PWM計算、反転フラグ） |
| MotorDriver実装 | ✅ | PWM+方向ピン、反転フラグ対応 |

### Phase 4: 統合

| タスク | ステータス | 備考 |
|--------|----------|------|
| DifferentialKinematicsテスト | ✅ | 10テストケース（cmd_vel→RPM変換） |
| DifferentialKinematics実装 | ✅ | キネマティクス計算 |
| MotorControllerテスト | ✅ | 13テストケース（目標RPM計算、回転優先クランプ） |
| MotorController実装 | ✅ | ロジック＋ハードウェア統合（実機確認は別途） |
| SharedMotorData定義 | ⬜ | コア間共有データ（linear_x, angular_z） |
| main.cpp Core0実装 | ⬜ | cmd_vel受信、新プロトコル対応 |
| main.cpp Core1実装 | ⬜ | setup1()/loop1() |

### Phase 5: 設定機能

| タスク | ステータス | 備考 |
|--------|----------|------|
| ConfigStorage実装 | ⬜ | Flash読み書き |
| GET/SET_CONFIG実装 | ⬜ | protocol.md参照、ROS経由で設定変更 |

### Phase 6: クリーンアップ

| タスク | ステータス | 備考 |
|--------|----------|------|
| CugoSDK削除 | ⬜ | lib/CugoSDKディレクトリ |
| SerialProtocol更新 | ⬜ | 新プロトコルに合わせて修正 |
| MotorLogic整理 | ⬜ | 製品ID依存コード見直し |
| ドキュメント最終更新 | ⬜ | |

## 次に着手すべきタスク

**→ Phase 4: SharedMotorData定義、main.cpp実装**

理由: MotorControllerのロジック実装が完了したので、コア間通信とmain.cppのデュアルコア対応を実装する。

## 完了履歴

| 日付 | タスク |
|------|--------|
| 2025-12-11 | architecture.md作成 |
| 2025-12-11 | CLAUDE.md更新 |
| 2025-12-11 | test_specifications.md作成 |
| 2025-12-11 | progress.md作成 |
| 2025-12-11 | protocol.md作成（リクエスト・レスポンス型通信仕様） |
| 2025-12-11 | development.md更新 |
| 2025-12-13 | HardwareConfig作成 |
| 2025-12-13 | CLAUDE.md コーディング規約追加（Arduinoスタイル） |
| 2025-12-13 | DebugLogger追加（Debug Probe対応） |
| 2025-12-13 | pico_debug環境追加（platformio.ini） |
| 2025-12-13 | PIDController テスト作成（19テストケース） |
| 2025-12-13 | PIDController 実装完了（アンチワインドアップ対応） |
| 2025-12-13 | development.md Homebrew版PlatformIO手順追加 |
| 2025-12-13 | protocol.md cmd_vel対応（linear_x, angular_z入力に変更） |
| 2025-12-13 | architecture.md mermaid図に更新、キネマティクス計算追加 |
| 2025-12-13 | test_specifications.md DifferentialKinematicsテスト追加 |
| 2025-12-13 | ConfigStorage設定項目拡張（wheel_diameter, track_width追加） |
| 2025-12-13 | QuadratureEncoder テスト作成（29テストケース: RPM計算+4逓倍デコード） |
| 2025-12-13 | QuadratureEncoder ロジック実装（calculateRpm, decodeState） |
| 2025-12-13 | MotorDriver テスト作成（11テストケース: 速度クランプ、方向判定、PWM計算） |
| 2025-12-13 | MotorDriver 実装（clampSpeed, getDirection, calculatePwmDuty） |
| 2025-12-13 | MotorDriver 反転フラグ追加（差動二輪のモータ取り付け方向対応） |
| 2025-12-13 | QuadratureEncoder 反転フラグ追加（decodeStateで符号反転） |
| 2025-12-13 | DifferentialKinematics テスト作成（10テストケース: cmd_vel→RPM変換） |
| 2025-12-13 | DifferentialKinematics 実装（キネマティクス計算） |
| 2025-12-13 | MotorController テスト作成（13テストケース: 目標RPM、回転優先クランプ） |
| 2025-12-13 | MotorController 実装（ロジック＋ハードウェア統合） |
