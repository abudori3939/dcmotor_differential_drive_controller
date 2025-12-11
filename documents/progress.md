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
| HardwareConfig作成 | ⬜ | ピンアサイン定義 |

### Phase 2: PIDController（TDD）

| タスク | ステータス | 備考 |
|--------|----------|------|
| テスト作成 | ⬜ | test_specifications.md参照 |
| PIDController実装 | ⬜ | |
| テストパス確認 | ⬜ | |

### Phase 3: エンコーダ・モータドライバ

| タスク | ステータス | 備考 |
|--------|----------|------|
| QuadratureEncoder RPM計算テスト | ⬜ | ロジック部分 |
| QuadratureEncoder実装 | ⬜ | 割り込み処理含む |
| MotorDriver実装 | ⬜ | PWM+方向ピン |

### Phase 4: 統合

| タスク | ステータス | 備考 |
|--------|----------|------|
| MotorController実装 | ⬜ | Core1で実行 |
| SharedMotorData定義 | ⬜ | コア間共有データ |
| main.cpp Core0実装 | ⬜ | ROS通信側、新プロトコル対応 |
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

**→ HardwareConfig作成**

理由: 他のライブラリがピン定義を参照するため、最初に作成する必要がある。

## 完了履歴

| 日付 | タスク |
|------|--------|
| 2024-12-11 | architecture.md作成 |
| 2024-12-11 | CLAUDE.md更新 |
| 2024-12-11 | test_specifications.md作成 |
| 2024-12-11 | progress.md作成 |
| 2024-12-11 | protocol.md作成（リクエスト・レスポンス型通信仕様） |
| 2024-12-11 | development.md更新 |
