# 作業レポート（2026-02-02）

## 概要
DX22_Project にて、敵・攻撃・当たり判定可視化・影・敵HP表示を実装。

## 実装内容
- 敵の追加と衝突ダメージ
  - 敵クラスを新規作成し、プレイヤー接触でHPが減るようにした。
  - 敵は静止、サイズはプレイヤーに合わせて同期。
- 攻撃の追加（Fキー）
  - Fキーで短時間の攻撃判定を生成。
  - 攻撃範囲はキャラ1.5体分程度の前方AABB。
  - 判定の見える化として Star.png を床面に表示。
- 敵HP・撃破
  - 敵にHPを持たせ、攻撃で1ダメージ。
  - HPが0になったら敵を削除。
- 当たり判定の可視化（AddLine）
  - 通常判定：緑（プレイヤー/敵）
  - 攻撃判定：赤
- 影の描画
  - Shadow.png を用いて床面に影スプライトを描画。
  - スプライト描画は深度テストOFF、距離順ソートで自然な重なりに。
- 敵HPの頭上表示
  - Number/num.png を使った数値表示を追加。
  - 敵の頭上位置をスクリーン座標に投影して2D描画。

## 追加/更新ファイル
- 新規
  - DX22_Project\Enemy.h
  - DX22_Project\Enemy.cpp
- 更新
  - DX22_Project\SceneGame.h
  - DX22_Project\SceneGame.cpp
  - DX22_Project\ScoreLite.h
  - DX22_Project\ScoreLite.cpp
  - DX22_Project\DX22_Project.vcxproj
  - DX22_Project\DX22_Project.vcxproj.filters

## 動作仕様メモ
- 攻撃判定は一定時間のみ有効。
- 1回の攻撃で1ダメージ（同スイングで多段ヒットしない）。
- 敵HPは画面上の数値で表示。

## 既知の調整ポイント
- 攻撃範囲/持続時間/ダメージ量は調整可能。
- 敵HP表示のサイズ・オフセットは調整可能。

## 次の候補
- 攻撃クールタイムやSE追加
- 敵AI（追従/徘徊）
- 敵死亡演出やドロップ


---

# 作業レポート（2026-02-06）

## 概要
カメラの描画範囲（フラスタム）表示と、ゲーム/デバッグの2カメラ切替を実装。ImGui からモード切替と両カメラの位置編集が可能。

## 実装内容
- カメラフラスタムの可視化（AddLine）
  - カメラの near/far・FOV・aspect からフラスタムをワイヤー描画。
- ゲーム/デバッグ2カメラの導入
  - ゲーム用とデバッグ用のカメラ状態を分離。
  - デフォルトはゲームモード。
- ImGui でのモード切替と位置編集
  - Camera タブに `Camera Mode`（Game/Debug）を追加。
  - Game/Debug それぞれの Eye/Look を個別に編集可能。
- デバッグモード中のフラスタム表示
  - デバッグモード時にゲームカメラのフラスタムを表示。
- 補助対応
  - `Camera::GetPos/GetLook` を `const` 化。
  - `Player::SetCamera` の未定義リンクエラーを解消。
  - `CameraDebug::SetPose`/軌道同期を追加。

## 追加/更新ファイル
- 更新
  - DX22_Project\Camera.h
  - DX22_Project\Camera.cpp
  - DX22_Project\CameraDebug.h
  - DX22_Project\CameraDebug.cpp
  - DX22_Project\Transfer.h
  - DX22_Project\SceneGame.h
  - DX22_Project\SceneGame.cpp
  - DX22_Project\Scene3DEditor.h
  - DX22_Project\Scene3DEditor.cpp
  - DX22_Project\Main.cpp
  - DX22_Project\Player.cpp

## 追加メモ
- `SceneGame.cpp` が一度 0 バイト化していたため、git の HEAD から復元して修正を再適用。

## 動作仕様メモ
- デバッグモード時: ゲームカメラの描画範囲線を表示。
- ゲームモード時: アクティブカメラの範囲線を表示。


---

# 追記（2026-02-07）

## 実施内容
- 工数・設計.md を追加。
  - ゲーム概要、現状実装、足りないアクション要素、設計方針、工数見積、MVP進行案を整理。

## 追加/更新ファイル
- 新規
  - 工数・設計.md


---

# 追記（2026-02-10）

## 差異（レポートと現状）
- 敵は静止ではなく、ステージ内を徘徊しプレイヤー接近で追従する簡易AI（Wander/Chase）。
- 敵HP表示は数値ではなく、`UIFrame.png` / `UIGauge.png` を使ったビルボード型ゲージ。
- プレイヤーHPの2DゲージUIを追加（UIObjectManagerで管理）。
- プレイヤーにダッシュ（Shift）を実装。距離/クールタイム/持続時間をパラメータ化し、ステージ外へ出ないようクランプ。
- プレイヤーにトレイル描画（TrailEffect）を追加。
- シーン構成に Title / Result / 3DEditor があり、EnterでTitle→Game、ImGuiの "Change Scene" で切替可能（初期はGame）。
- ImGui の監視/デバッグUI（Player/Model/Camera/Dice等の調整、Table/Overlay表示）を追加。

## 補足
- Goal / Dice / Yukari / Tyabudai などのクラス・素材は存在するが、SceneGameでの生成は未接続（現状は未使用）。


---

# 追記（2026-02-13）

## 実施内容（今回）
- `SceneGame` の Goal を再接続。
  - Goal を生成し、カメラ追従設定とステージ内配置を実施。
  - プレイヤーが Goal に接触したら `ResultType::Win` を設定して `SCENE_RESULT` へ遷移。
- 敗北導線を追加。
  - プレイヤーHPが0以下になった時に `ResultType::Lose` を設定して `SCENE_RESULT` へ遷移。
- F攻撃を「前方固定」から「薙ぎ払い」に変更。
  - 攻撃有効時間内で当たり判定中心が左前→正面→右前へ弧を描いて移動する方式に変更。
  - 可視化（赤線AABB / Starマーカー）は既存のまま利用。

## 当たり判定方針（AABB統一）
- 今後、このプロジェクトのゲームプレイ当たり判定は **AABB（軸平行ボックス）** を基準に統一する。
- `Collision::Box` + `Collision::Hit(Box, Box)` を標準ルートとする。
- OBB系（`RigidBodyOBB` など）は既存検証・別用途コードとして残すが、ゲームプレイ判定には使わない。
- モデルの見た目が回転していても、当たり判定はワールド軸に平行なAABBで扱う。

## 追加/更新ファイル
- 更新
  - DX22_Project\SceneGame.cpp
  - WORK_REPORT_2026-02-02.md


---

# 追記（2026-02-13 / 継続）

## 実施内容（1〜5を順に実施）
- 1) ビルド復旧
  - `DX22_Project.vcxproj.user` の先頭に重複BOMが入っていたため修正。
  - さらに `*.cpp/*.h` の一部で重複BOM（`EF BB BF EF BB BF`）が混入しており、先頭3バイト除去で正規化。
  - `Debug|x64` ビルドが通る状態に復旧。
- 2) AABB方針の実装ルール化
  - `SceneGame` 内のゲーム判定呼び出しを `HitAabb` ヘルパー経由に統一。
  - Player/Enemy、Attack/Enemy、Player/Goal 判定を AABB ルートへ集約。
- 3) 薙ぎ払い攻撃のゲーム化
  - F攻撃に `windup / active / recovery / cooldown` のタイマー管理を追加。
  - 連打で攻撃が重ならないようにし、攻撃テンポを調整しやすい構造に変更。
  - 調整値: `windup=0.04s / active=0.12s / recovery=0.10s / cooldown=0.24s`。
- 4) 勝敗ループの補強
  - Resultシーンで Enter = 再挑戦（Gameへ）、`T` = Titleへ戻る導線を追加。
  - Result遷移時に `ResultType::None` を明示リセット。
- 5) 敵攻撃の予兆/クールタイム
  - 敵接触即ダメージを廃止し、`予兆(windup)` 後にダメージ判定を入れる方式に変更。
  - 敵攻撃にクールタイムを追加。
  - デバッグAABB色で状態可視化（予兆中=橙、攻撃可能=黄、通常=緑）。
  - 調整値: `enemy windup=0.55s / cooldown=1.00s / rangeScale=1.35`。

## 追加/更新ファイル
- 更新
  - DX22_Project\SceneGame.h
  - DX22_Project\SceneGame.cpp
  - DX22_Project\SceneResult.cpp
  - DX22_Project\DX22_Project.vcxproj.user
  - DX22_Project\Camera.h
  - DX22_Project\Camera.cpp
  - DX22_Project\CameraDebug.h
  - DX22_Project\CameraDebug.cpp
  - DX22_Project\Dice.cpp
  - DX22_Project\DirectX.cpp
  - DX22_Project\GaugeUI.cpp
  - DX22_Project\Player.cpp
  - DX22_Project\Scene3DEditor.h
  - DX22_Project\Scene3DEditor.cpp
  - DX22_Project\Startup.cpp
  - DX22_Project\UIObject.h
  - DX22_Project\UIObject.cpp
  - DX22_Project\_geometory.cpp
  - WORK_REPORT_2026-02-02.md


---

# 追記（2026-02-14）

## 実施内容
- ImGui実行中にゲーム調整値を変更できるように、`Transfer` に `GameplayTuning` を追加。
  - 攻撃（windup/active/recovery/cooldown、薙ぎ払い角度/半径/サイズ）
  - 敵攻撃（予兆/クールタイム/射程/ダメージ）
  - 押し合い（slop / playerShare / enemyShare）
  - 敵数（`enemyCount`）
- Mainの `Main Setting Window` に `Gameplay` タブを追加し、上記をリアルタイム調整可能化。
- `SceneGame` 側は直書き定数ではなく `tran.gameplay` を毎フレーム参照するよう変更。
  - 実行中のImGui操作がそのまま挙動に反映される。
  - 敵数も実行中に増減反映（spawn/despawn）。
- 現状未使用のタブを削除。
  - 削除: `Dice`, `Dice 4X4`, `Dice drop`, `Yukari`, `Tyabudai`

## 今後の運用ルール（ImGui調整）
- これ以降に追加するゲームプレイ数値は、原則として以下で管理する。
  1. `Transfer::GameplayTuning` にメンバ追加
  2. `Main.cpp` の `Gameplay` タブに操作UI追加
  3. 実処理側（Scene/Player/Enemy）は `tran.gameplay` を参照
- 直書き定数は「物理的に固定の値（例: 画面UI余白など）」に限定する。

## 追加/更新ファイル
- 更新
  - DX22_Project\Transfer.h
  - DX22_Project\Main.cpp
  - DX22_Project\SceneGame.h
  - DX22_Project\SceneGame.cpp
  - WORK_REPORT_2026-02-02.md


---

# 追記（2026-02-14 / 追加実装）

## 実施内容（6〜8を含む）
- 複数敵同時ヒット化
  - 薙ぎ払い攻撃が「当たった敵全員」にダメージするよう変更。
  - 同一スイング内の多段ヒット防止は敵ごとの `swingId` 管理で厳密化。
- 敵同士の押し合い
  - AABB重なり時に敵同士をX/Z最小貫通軸で分離。
  - ステージ外へ出ないようクランプ。
- 攻撃の手応え強化
  - プレイヤー攻撃ヒット時にヒットストップ、ノックバック、ヒットフラッシュを追加。
  - すべて `Gameplay` タブから実行中調整可能にした。
- 攻撃SE導入
  - `Assets/Sound/attack.mp3` をロードし、プレイヤー攻撃ヒット時に再生。
  - `InitSound/UninitSound` をアプリ初期化/終了に接続。
- 敵AIのばらけ行動
  - 追尾ターゲットに分離ベクトルを加算し、密集しにくい挙動に変更。
  - 分離半径・重み・最大オフセットを ImGui で調整可能化。
- 敵スポーン設計
  - スポーン時に候補探索を行い、プレイヤー/既存敵との最小距離を満たす位置を優先採用。
  - リング半径・ジッタ・最小距離を ImGui で調整可能化。
- ImGui整理
  - `Gameplay` タブの各グループにデフォルト値表示を追加。
  - リセットボタンを `Apply Default Gameplay Values` に改名。
- 当たり判定デバッグ表示の強化
  - 敵AABB色を状態別で可視化（被弾/予兆/攻撃可能/射程内クール/射程外）。
  - デバッグカメラ時のみ敵攻撃レンジ枠を表示。
  - `Gameplay` タブに色凡例を追加。
- ゲームループ調整（敵全滅時）
  - 方針を「敵全滅でクリア」に決定し実装。
  - 敵数自動補充は停止し、`enemyCount` 変更時のみ再構成する方式に変更。

## 当たり判定方針（再確認）
- ゲームプレイ判定は引き続き AABB 統一。
- 追加した押し合い・攻撃ヒット・全滅判定周辺も AABB ベースで実装。

## 追加/更新ファイル
- 更新
  - DX22_Project\Main.cpp
  - DX22_Project\Transfer.h
  - DX22_Project\SceneGame.h
  - DX22_Project\SceneGame.cpp
  - WORK_REPORT_2026-02-02.md


---

# 追記（2026-02-14 / セッション総括）

## 今までにやったこと（要約）
- 当たり判定ポリシーを AABB 統一で運用。
- 敵を複数体管理に変更し、プレイヤー攻撃を「ヒットした敵全員」に適用。
- プレイヤー/敵、敵/敵の押し合い（重なり解消）を追加。
- 攻撃手応えを強化（ヒットストップ、ノックバック、ヒットフラッシュ）。
- 攻撃SEを導入（`Assets/Sound/attack.mp3`）。
- 多段ヒット防止を `swingId` 管理で厳密化。
- 敵AIの分離行動（密集回避）を追加。
- スポーン設計を改善（プレイヤー/敵との最小距離を考慮した候補探索）。
- `Gameplay` タブを拡張し、追加項目を実行中に調整可能化。
- デバッグ表示を強化（敵状態色分け、デバッグカメラ時の攻撃レンジ表示、凡例）。
- ゲームループを調整し、「敵全滅でクリア」仕様を実装。
  - `enemyCount` は変更時のみ再構成（自動補充しない）。

## 次にやるべきこと（優先順）
1. Wave制の導入（最優先）
   - 全滅後に次Waveへ遷移し、Waveごとに敵数/敵性能を段階的に上げる。
2. ゲームUIの明確化
   - 「現在Wave」「残り敵数」「クリア条件」を常時表示。
3. 敵の種類追加
   - 速度型/耐久型/遠距離型など2〜3種類を追加して戦術差を作る。
4. 難易度プリセット化
   - Easy/Normal/Hard をワンボタンで切替可能にする。
5. サウンド拡張
   - 被弾SE/敵攻撃SE/クリアSE/BGMの追加と音量調整UIの実装。
6. エフェクト強化
   - 敵撃破演出、被弾演出、攻撃軌跡の視覚強化。
7. 設定保存
   - `Gameplay` パラメータの保存/読込（起動時復元）。
8. テスト項目の固定化
   - 当たり判定・全滅勝利・SE再生・敵数変更時再構成の確認表を作成。


---

# 追記（2026-02-14 / 作業再開）

## 実施内容
- Wave制を実装。
  - 敵全滅時に `Wave` を進行し、最終Wave全滅時のみ `ResultType::Win` で `SCENE_RESULT` へ遷移。
  - `waveMax` / `waveEnemyAddPerWave` から Waveごとの敵数を算出。
- Waveごとの敵性能段階化を実装。
  - `enemyMoveSpeed` を基準に、Wave進行で `waveEnemyMoveSpeedAdd` を加算。
  - `enemyAttackDamage` を基準に、Wave進行で `waveEnemyAttackDamageScalePerWave` を乗算加算。
- ImGui `Gameplay` タブに Wave関連調整項目を追加。
  - `Wave Max`, `Wave Enemy Add`, `Enemy Move Speed`, `Wave MoveSpeed Add`, `Wave Damage Scale`
- Runtime Debug と Inspector に Wave監視値を追加。
  - `currentWave`, `maxWave`, `enemiesAlive`, `enemiesTarget`

## 追加/更新ファイル
- 更新
  - DX22_Project\Transfer.h
  - DX22_Project\SceneGame.h
  - DX22_Project\SceneGame.cpp
  - DX22_Project\Main.cpp
  - WORK_REPORT_2026-02-02.md


---

# 追記（2026-02-14 / ローグライク化）

## 実施内容
- 難易度がゲーム進行へ与える実効補正を追加。
  - 難易度ごとに「基準敵数」「Waveごとの追加敵数」「敵攻撃ダメージ倍率」を補正する方式へ変更。
  - `Gameplay` の設定値は基準値として残し、実行時に実効値を算出。
- ステージクリア報酬によるプレイヤー強化を追加。
  - 強化系統: 攻撃力 / 攻撃頻度 / 回避CT短縮。
  - 強化状態は実行時デバッグ・インスペクター・専用タブで確認可能。
- ダッシュを「回避」扱いへ整理。
  - パラメータ表記を回避に変更（距離/CT/時間）。
  - 回避中はプレイヤーと敵の押し合い解決を無効化し、敵をすり抜け可能に変更。
  - 回避中は敵攻撃ヒット判定を無効化（被弾しない）。

## 追加/更新ファイル
- 更新
  - DX22_Project\Player.h
  - DX22_Project\Player.cpp
  - DX22_Project\SceneGame.cpp
  - DX22_Project\Main.cpp
  - DX22_Project\Transfer.h
  - DX22_Project\Transfer.cpp
  - WORK_REPORT_2026-02-02.md


---

# 追記（2026-02-14 / 強化選択制）

## 実施内容
- 勝利時の強化付与を「自動付与」から「三択選択」へ変更。
  - `ResultType::Win` 時に強化選択待ち状態へ遷移。
  - `1/2/3` キーで候補を1つ選択して適用後、次プレイへ進行。
- リロール機能を追加。
  - `R` キーで候補を再抽選。
  - リロールには上限を設け、残回数が0なら再抽選不可。
  - 上限値は ImGui の `ゲーム調整` から変更可能。
- 強化候補表示を追加。
  - Winリザルト画面中央に候補3つと説明文、リロール残回数を表示。
  - `強化状態` タブにも候補と残回数を表示。
- 保存/読込に強化関連項目を追加。
  - 強化レベル、最終取得強化、リロール上限を `gameplay_tuning.cfg` へ保存/復元。

## 操作
- 勝利時:
  - `1` / `2` / `3`: 候補選択
  - `R`: リロール（残回数がある場合のみ）

## 追加/更新ファイル
- 更新
  - DX22_Project\SceneResult.cpp
  - DX22_Project\SceneGame.cpp
  - DX22_Project\Main.cpp
  - DX22_Project\Transfer.h
  - DX22_Project\Transfer.cpp
  - WORK_REPORT_2026-02-02.md


---

# 追記（2026-02-14 / 現時点総括）

## 今までにやったこと（最新版）
- ゲームプレイ当たり判定を AABB で統一し、判定経路を整理。
- Wave進行と敵の段階強化（速度/攻撃）を実装。
- 敵タイプ（速度型/耐久型/遠距離型）を追加。
- 難易度プリセット（Easy/Normal/Hard）を実装。
- サウンド拡張（BGM/SE、音量調整、SE後始末）を実装。
- サウンド構成を `Assets/Sound/BGM` と `Assets/Sound/SE` に分離。
- 最終Wave到達時のボスBGM切替（`GameBGM2.mp3`）を実装。
- エフェクト強化（攻撃軌跡、被弾、撃破）を実装。
- 調整値の保存/読込（`Assets/gameplay_tuning.cfg`）を実装。
- テスト確認表を固定化（`TEST_CHECKLIST_2026-02-14.md`）。
- 回避仕様を強化（回避中の貫通・押し合い無効・被弾無効）。
- ローグライク強化を導入し、勝利時に三択選択＋リロール制へ移行。


---

# 追記（2026-02-14 / サウンド拡張 追補2）

## 実施内容
- SEファイル配置を `Assets/Sound/SE` へ分離。
  - `attack.mp3`
  - `player_hit.mp3`
  - `enemy_attack.mp3`
  - `clear.mp3`
- `SceneGame` のSE読込パスを `Assets/Sound/SE/*.mp3` に変更。
- ボス戦BGM切替を実装。
  - `GameBGM2.mp3` を読込。
  - 最終Wave到達時（`currentWave >= waveMax`）に通常BGMから `GameBGM2.mp3` へ切替。
- 実行フォルダ（`x64/Debug/Assets/Sound`）へ `BGM/SE` 構成を同期し、起動確認を実施。

## 追加/更新ファイル
- 更新
  - DX22_Project\SceneGame.h
  - DX22_Project\SceneGame.cpp
  - TEST_CHECKLIST_2026-02-14.md
  - WORK_REPORT_2026-02-02.md


---

# 追記（2026-02-14 / 作業再開8）

## 実施内容
- 優先項目8「テスト項目の固定化」を実施。
  - 固定チェックシートを新規作成。
  - 必須8ケース（AABB/勝敗/Wave/SE/BGM/敵数再構成）を定義。
  - 優先項目7の保存/読込検証ケースも追加。
  - 結果記録フォーマット（`OK / NG / 未実施`）を統一。
  - 固定チェックシートへ今回の判定結果を記入（コード経路確認＋Transfer単体実測）。

## テストシート
- `TEST_CHECKLIST_2026-02-14.md`

## 追加/更新ファイル
- 新規
  - TEST_CHECKLIST_2026-02-14.md
- 更新
  - WORK_REPORT_2026-02-02.md


---

# 追記（2026-02-14 / サウンド拡張 追補）

## 実施内容
- `Gameplay` タブに音量調整を追加。
  - `Master音量 / BGM音量 / SE音量`
- 音量設定を毎フレーム反映するよう変更。
  - BGM: Game/Resultで再生中のVoiceへ反映
  - SE: 再生開始時に音量適用
- `Sound`に1ショットSEの後始末を追加。
  - 再生終了したSourceVoiceを `UpdateSound` で破棄し、累積負荷を抑制。
- SE再生ポイントを拡張。
  - 被弾SE: `Assets/Sound/player_hit.mp3`
  - 敵攻撃SE: `Assets/Sound/enemy_attack.mp3`
  - クリアSE: `Assets/Sound/clear.mp3`
  - ※ファイル未配置時は再生スキップ（ゲーム進行は継続）
- 設定保存/読込（`gameplay_tuning.cfg`）に音量項目を追加。

## 追加/更新ファイル
- 更新
  - DX22_Project\Sound.h
  - DX22_Project\Sound.cpp
  - DX22_Project\Main.cpp
  - DX22_Project\SceneGame.cpp
  - DX22_Project\SceneResult.cpp
  - DX22_Project\Transfer.h
  - DX22_Project\Transfer.cpp
  - WORK_REPORT_2026-02-02.md


---

# 追記（2026-02-14 / 作業再開7）

## 実施内容
- 優先項目7「設定保存」を実装。
  - `Transfer` にゲーム調整パラメータの保存/読込APIを追加。
  - 保存先: `Assets/gameplay_tuning.cfg`
  - 起動時に自動読込（存在する場合）。
  - 終了時に自動保存。
- ImGui `ゲーム調整` タブに手動操作を追加。
  - `設定を保存`
  - `設定を読込`
  - 保存先パス表示
- 既存の `ゲーム調整を初期値に戻す` は `Transfer` の初期値復元関数を使う形に整理。

## 追加/更新ファイル
- 更新
  - DX22_Project\Transfer.h
  - DX22_Project\Transfer.cpp
  - DX22_Project\Main.cpp
  - WORK_REPORT_2026-02-02.md


---

# 追記（2026-02-14 / 作業再開2）

## 実施内容
- 優先項目2「ゲームUIの明確化」を実装。
  - `SCENE_GAME` 中に、画面左上へ以下を常時表示するHUDを追加。
    - 現在Wave（`currentWave / maxWave`）
    - 残り敵数（`enemiesAlive / enemiesTarget`）
    - クリア条件（最終Waveで敵を全滅）
  - 表示は ImGui DrawList の前景レイヤで描画。
  - 既存のF2オーバーレイ切替とは独立して常時表示。

## 追加/更新ファイル
- 更新
  - DX22_Project\Main.cpp
  - WORK_REPORT_2026-02-02.md


---

# 追記（2026-02-14 / 作業再開3）

## 実施内容
- 優先項目3「敵の種類追加」を実装。
  - 敵タイプを3種追加。
    - 速度型（低HP・高機動・短CT寄り）
    - 耐久型（高HP・低機動・高ダメージ寄り）
    - 遠距離型（射程長め・低ダメージ寄り）
  - スポーン時にタイプを割当（速度型/耐久型/遠距離型を循環）。
  - タイプごとに以下を補正。
    - HP
    - 移動速度倍率
    - 敵攻撃の射程/予兆/クールタイム/ダメージ倍率
- Inspector に `enemy.type` を追加。
  - `0=速度型 / 1=耐久型 / 2=遠距離型` で表示。

## 追加/更新ファイル
- 更新
  - DX22_Project\Enemy.h
  - DX22_Project\Enemy.cpp
  - DX22_Project\SceneGame.cpp
  - DX22_Project\Transfer.h
  - DX22_Project\Main.cpp
  - WORK_REPORT_2026-02-02.md


---

# 追記（2026-02-14 / 作業再開4）

## 実施内容
- 優先項目4「難易度プリセット化」を実装。
  - `Gameplay` タブに `Easy / Normal / Hard` のワンボタン適用を追加。
  - プリセット適用時に、敵数・Wave数・敵攻撃・敵移動・Wave増加補正をまとめて反映。
  - 実行時デバッグに現在難易度を表示。
  - ゲーム中の常時HUDにも難易度表示を追加。
  - Inspector に `gameplay.difficultyPreset` を追加。

## 追加/更新ファイル
- 更新
  - DX22_Project\Transfer.h
  - DX22_Project\Main.cpp
  - WORK_REPORT_2026-02-02.md


---

# 追記（2026-02-14 / 作業再開5）

## 実施内容
- 優先項目5「サウンド拡張」の第1段階としてBGMを導入。
  - `SceneGame` 開始時に `Assets/Sound/BGM/GameBGM.mp3` をループ再生。
  - `SceneResult` 開始時に `Assets/Sound/BGM/ResultBGM.mp3` をループ再生。
  - シーン破棄時に各BGM Voiceを停止・破棄。
- `GameBGM2.mp3` はボス戦用素材として配置のみ（当時は未使用、追補2で最終Wave切替を実装）。

## 追加/更新ファイル
- 更新
  - DX22_Project\SceneGame.h
  - DX22_Project\SceneGame.cpp
  - DX22_Project\SceneResult.h
  - DX22_Project\SceneResult.cpp
  - WORK_REPORT_2026-02-02.md


---

# 追記（2026-02-14 / 作業再開6）

## 実施内容
- 優先項目6「エフェクト強化」を実装。
  - 攻撃軌跡の残像エフェクトを追加（薙ぎ払い中にマーカー残像を生成）。
  - 被弾演出を追加（敵攻撃ヒット時にプレイヤー周辺へ赤系フラッシュ）。
  - 敵撃破演出を追加（敵死亡時に金系フラッシュ）。
- ImGui `ゲーム調整` タブに演出調整パラメータを追加。
  - 攻撃軌跡: 間隔 / 残存時間 / サイズ倍率
  - 被弾演出: フラッシュ時間 / サイズ倍率
  - 撃破演出: フラッシュ時間 / サイズ倍率
- `ゲーム調整を初期値に戻す` に上記パラメータの初期化を追加。

## 追加/更新ファイル
- 更新
  - DX22_Project\Transfer.h
  - DX22_Project\SceneGame.h
  - DX22_Project\SceneGame.cpp
  - DX22_Project\Main.cpp
  - WORK_REPORT_2026-02-02.md


---

# 追記（2026-02-15 / 作業再開9）

## 実施内容
- 敵タイプ差を強化し、遠距離型を「敵弾発射」方式へ変更。
  - 予兆完了時、遠距離型は即時接触ダメージではなく敵弾を生成。
  - 敵弾は移動・寿命・AABB被弾判定を持ち、プレイヤー接触時にダメージ。
  - 回避中は既存仕様どおり被弾無効。
- 敵弾の実行時調整を追加。
  - `Gameplay` タブに `敵弾速度 / 敵弾寿命 / 敵弾半径 / 敵弾ダメ倍率` を追加。
  - 遠距離型の挙動説明をデバッグ表示に追記。
- 設定保存/読込を拡張。
  - `gameplay_tuning.cfg` に敵弾4項目を保存・復元。
- デバッグ可視化を追加。
  - 敵弾をシーン上マーカーで表示。
  - デバッグカメラ時に敵弾AABB線を表示。
- ビルド確認。
  - `Debug|x64` ビルド成功（エラー0）。

## 追加/更新ファイル
- 更新
  - DX22_Project\SceneGame.h
  - DX22_Project\SceneGame.cpp
  - DX22_Project\Transfer.h
  - DX22_Project\Transfer.cpp
  - DX22_Project\Main.cpp
  - TEST_CHECKLIST_2026-02-14.md
  - WORK_REPORT_2026-02-02.md


---

# 追記（2026-02-15 / 差分確認）

## 確認結果（レポート記載と現状コードの差分）
- 大きな不整合はなし（主要項目は一致）。
- 差分として確認された2点（勝利条件の二重化、`gameplay_tuning.cfg` の2配置差分）を次項で対応。

## 差分対応（2026-02-15 / 作業再開10）
1. 勝利条件の仕様を一本化。
   - `SceneGame` の Goal接触時Win遷移を撤去し、勝利は「最終Wave全滅時のみ」に統一。
2. `gameplay_tuning.cfg` の運用を一本化。
   - `Transfer::LoadGameplayTuning` にフォールバック読込を追加。
   - `Transfer::SaveGameplayTuning` を既定時の複数パス同期保存に変更。
   - 同期対象: `Assets/gameplay_tuning.cfg` / `x64/Debug/Assets/gameplay_tuning.cfg` / `DX22_Project/Assets/gameplay_tuning.cfg`
   - 初回整合として `DX22_Project/Assets/gameplay_tuning.cfg` を `x64/Debug/Assets/gameplay_tuning.cfg` へ同期し、差分ゼロを確認。
3. テストケースを追加。
   - `TEST_CHECKLIST_2026-02-14.md` に `T15/T16/T17`（勝利条件統一、強化三択、リロール）を追加。
4. ビルド確認手順を再整備。
   - ルートに `build_debug_x64.ps1` を追加（`msbuild` 直接検出 + `vswhere` フォールバック）。
5. 反映確認を実施。
   - `build_debug_x64.ps1` 実行で `Debug|x64` ビルド成功（警告0 / エラー0）。

## 追加/更新ファイル
- 更新
  - DX22_Project\SceneGame.cpp
  - DX22_Project\Transfer.cpp
  - TEST_CHECKLIST_2026-02-14.md
  - WORK_REPORT_2026-02-02.md
- 新規
  - build_debug_x64.ps1


---

# 追記（2026-02-15 / 整合性再確認）

## 依頼内容
- `WORK_REPORT_2026-02-02.md` と `TEST_CHECKLIST_2026-02-14.md` の記載内容と、現状コードの進行内容の整合性を再確認。

## 差分（確認結果）
1. テスト項目の文言差分（Goal）
   - チェックシート `T01/T15` に Goal接触前提の文言が残っていた。
   - 現状コードは Goal未使用で、勝利条件は「最終Wave全滅時のみ」。
2. 設定ファイル運用の文言差分（`gameplay_tuning.cfg`）
   - チェックシート `T09/T10` の備考が3系統表記だった。
   - 現状コードの既定運用はフォールバック/同期ともに4系統（`../../DX22_Project/Assets` を含む）。

## 差分対応（実施）
- `TEST_CHECKLIST_2026-02-14.md` を更新。
  - `T01/T15` を Goal未使用の現仕様に合わせて修正。
  - `T09/T10` のパス記載を4系統に修正。
- 主要実装の整合性は維持されていることを再確認。
  - 勝利条件一本化（最終Wave全滅のみ）
  - Win時三択 + リロール
  - 遠距離型の敵弾 + AABB被弾判定
  - `build_debug_x64.ps1` で `Debug|x64` ビルド成功

## 追加/更新ファイル
- 更新
  - TEST_CHECKLIST_2026-02-14.md
  - WORK_REPORT_2026-02-02.md


---

# 追記（2026-02-15 / コミット・Push記録）

## 実施内容
- 変更を「ドキュメント」と「実装」で分割してコミット。
  1. `1d8fd74` `docs: sync report/checklist with current task progress`
     - 対象: `WORK_REPORT_2026-02-02.md` / `TEST_CHECKLIST_2026-02-14.md`
  2. `da59db0` `feat: align gameplay systems and build workflow updates`
     - 対象: `Main.cpp` / `SceneGame.*` / `Transfer.*` / `DX22_Project.vcxproj*` / `Assets/gameplay_tuning.cfg` / `imgui.ini` / `build_debug_x64.ps1`
- `origin` の `ActionGame` ブランチへ Push 実施。
  - 反映結果: `ActionGame -> origin/ActionGame`
- `DX22_Project/Sound.h` は変更表示が出ていたため追加確認。
  - `git add` 後に差分なし判定となり、追加コミットは不要（`Everything up-to-date`）。

## 追加/更新ファイル
- 更新
  - WORK_REPORT_2026-02-02.md
