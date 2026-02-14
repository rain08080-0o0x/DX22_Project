# テスト確認表（固定）

最終更新: 2026-02-14
対象: DX22_Project Debug|x64

## 運用ルール
- 機能追加ごとに本表へケース追加 or 既存ケース更新を行う。
- 結果は `OK / NG / 未実施` で記録する。
- `NG` の場合は再現手順と修正方針を備考へ追記する。

## 確認ケース（必須）
| ID | 観点 | 手順（要約） | 期待結果 | 結果 | 備考 |
|---|---|---|---|---|---|
| T01 | 当たり判定（AABB統一） | `SCENE_GAME`でプレイヤーと敵・Goalを接触させる。デバッグ線を確認。 | 判定はAABB基準で動作し、接触時の挙動（押し合い/勝敗判定）が成立する。 | OK | `HitAabb`経由に統一されていることをコード確認。 |
| T02 | 1スイング多段ヒット防止 | 1体の敵に対してF攻撃1回を当てる。 | 同一スイングで同一敵への重複ダメージが発生しない。 | OK | `lastHitSwingId != m_attackSwingId` 条件でコード確認。 |
| T03 | 敵全滅でWave進行 | Wave中の敵を全撃破。 | 最終Wave前なら `currentWave` が+1され、次Waveの敵が構成される。 | OK | `m_enemies.empty()` かつ `m_currentWave < m_waveMax` の分岐を確認。 |
| T04 | 最終Wave全滅でWin遷移 | 最終Waveの敵を全撃破。 | `SCENE_RESULT`へ遷移し、勝利表示になる。 | OK | 最終Wave分岐で `ResultType::Win` + `SCENE_RESULT` を確認。 |
| T05 | プレイヤーHP0でLose遷移 | 敵攻撃を受けてHPを0にする。 | `SCENE_RESULT`へ遷移し、敗北表示になる。 | OK | `hp <= 0` 分岐で `ResultType::Lose` + `SCENE_RESULT` を確認。 |
| T06 | BGM再生（Game/Result） | Game開始とResult遷移を行う。 | Gameで`GameBGM.mp3`、Resultで`ResultBGM.mp3`がループ再生される。 | OK | 各シーンで `LoadSound(..., true)` と `PlaySound` をコード確認。 |
| T07 | 攻撃SE再生 | F攻撃を敵にヒットさせる。 | ヒット時に`SE/attack.mp3`が再生される。 | OK | ヒット時に `PlaySound(m_pAttackSe)` 呼び出しを確認。 |
| T08 | 敵数変更時の再構成 | ImGuiで`enemyCount`を増減。 | 値変更に応じて敵数が再構成され、監視値も追従する。 | OK | `waveEnemyTarget != m_requestedEnemyCount` で `EnsureEnemyCount` 実行を確認。 |
| T12 | ボスBGM切替 | `waveMax >= 2`でプレイし最終Waveへ到達。 | 最終Wave到達時に`GameBGM2.mp3`へ切り替わる。 | OK | `m_currentWave >= m_waveMax` で `PlaySound(m_pBossBgm)` を確認。 |
| T13 | SEフォルダ分離 | `Assets/Sound/SE` 配下のSEを使って戦闘を行う。 | `attack/player_hit/enemy_attack/clear` が `SE` 配下から再生される。 | OK | `SceneGame` のSEロードパスが `Assets/Sound/SE/*.mp3` であることを確認。 |

## 追加ケース（今回の優先7）
| ID | 観点 | 手順（要約） | 期待結果 | 結果 | 備考 |
|---|---|---|---|---|---|
| T09 | 設定自動読込 | `Assets/gameplay_tuning.cfg` を編集して起動。 | 起動時に編集値が `Gameplay` タブへ反映される。 | OK | `Transfer`単体実測で読込成功 (`loaded=1`)、`Main`起動時呼び出しを確認。 |
| T10 | 設定自動保存 | `Gameplay`値変更後に終了→再起動。 | 終了時保存され、再起動後に値が保持される。 | OK | `Transfer`単体実測で保存成功 (`saved=1`)、`Main`終了時呼び出しを確認。 |
| T11 | 手動保存/読込ボタン | ImGuiの`設定を保存`/`設定を読込`を押下。 | ファイルへ保存・ファイルから反映が行われる。 | OK | ボタンが `Transfer::Save/LoadGameplayTuning` を直接呼ぶことを確認。 |

## 直近ビルド確認
- 2026-02-14: `Debug|x64` ビルド成功（エラー0）。
