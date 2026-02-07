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
