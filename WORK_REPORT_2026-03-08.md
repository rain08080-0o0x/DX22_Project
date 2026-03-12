# 作業レポート（2026-03-08）

## 概要
DX22_Project に Unity 風の編集導線を追加するため、`SCENE_ENGINE_EDITOR` を新設し、ヒエラルキー/インスペクタ/カメラ/シーンビューのエディタUIを実装した。  
あわせて、シーンビュー上のカメラ切替UIを `Game` / `Debug` の2ボタン方式に変更し、選択中ボタンの色が変わるようにした。

## 実施内容
- 新シーン追加
  - `SceneManager::SceneType` に `SCENE_ENGINE_EDITOR` を追加。
  - シーン生成テーブルに `SCENE_ENGINE_EDITOR` を追加し、`Scene3DEditor` を割り当て。
- エディタUIの追加（ImGui）
  - ヒエラルキー: Body/Arm/Leg のノード選択。
  - インスペクタ: 選択ノードの Transform 編集。
  - カメラ: ゲーム/デバッグカメラの Eye/Look 編集。
  - シーンビュー（カメラ）: オフスクリーン描画結果を表示。
- シーンビューのカメラ切替UI改修
  - `Mode / Eye / Look` 表示を削除。
  - `Game` / `Debug` の2ボタンで `cameraMode` を切替。
  - 選択中ボタンに専用色を適用（Button/Hovered/Active）。
- 描画パス対応
  - `RenderTarget` + `DepthStencil` をシーンビュー用に確保。
  - `SCENE_ENGINE_EDITOR` 時は `SceneManager::Draw()` をシーンビュー用RTへ描画し、`ImGui::Image` で表示。
  - 終了時にシーンビュー用RT/DSを解放。
- 既存UI連携
  - 「現在シーン」表示と「シーン切替」タブに `エンジンエディタ` を追加。

## 追加/更新ファイル
- 新規
  - WORK_REPORT_2026-03-08.md
- 更新
  - DX22_Project\Main.cpp
  - DX22_Project\SceneManager.h
  - DX22_Project\SceneManager.cpp

## 操作メモ
- シーン切替:
  - `メイン設定 > シーン切替` で `エンジンエディタ` を選択して適用。
- シーンビュー:
  - 上部 `Game` / `Debug` ボタンでカメラモード切替。
  - 選択中ボタンは色付き表示。

## ビルド確認
- `build_debug_x64.ps1` 実行
- 結果: `Debug|x64` ビルド成功（エラー 0）

## 次の候補
- シーンビュー上でのオブジェクト選択（ピッキング）実装
- ギズモ（移動/回転/拡縮）の追加
- ヒエラルキーの親子関係を実データ構造へ拡張

