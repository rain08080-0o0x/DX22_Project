#include "Main.h"
#include <memory>
#include "DirectX.h"
#include "Geometory.h"
#include "Sprite.h"
#include "Input.h"
#include "SceneManager.h"
#include "CastleSaveData.h"
#include "SceneCastleEditor.h"
#include "Defines.h"
#include "ShaderList.h"
#include "Transfer.h"
#include "Sound.h"
#include "Texture.h"
// rand初期化用
#include <cstdlib>
#include <ctime>
#include <cstdio> // 追加
#include <cmath>

// ImGui
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "Easing.h"

// デバッグ用
#include "DebugUtil.h"

namespace
{
	const char* GetSkillNameByType(int skillType)
	{
		switch (skillType)
		{
		case Transfer::RoguelikeUpgrade::SkillShot:
			return u8"遠距離攻撃";
		case Transfer::RoguelikeUpgrade::SkillNova:
			return u8"近接攻撃";
		case Transfer::RoguelikeUpgrade::SkillOrbit:
			return u8"衛星攻撃";
		default:
			return u8"未取得";
		}
	}

	const char* GetDifficultyName(int difficultyPreset)
	{
		switch (difficultyPreset)
		{
		case 0:
			return u8"Easy";
		case 2:
			return u8"Hard";
		default:
			return u8"Normal";
		}
	}

	void FormatUpgradeLabel(const Transfer& tran, int upgradeType, char* out, size_t outSize)
	{
		if (!out || outSize == 0) return;

		const int difficultyPreset = tran.NormalizeDifficultyPreset(tran.gameplayDebug.difficultyPreset);
		const int amount = tran.GetUpgradeStepForType(upgradeType, difficultyPreset);
		const char* name = nullptr;
		switch (upgradeType)
		{
		case 0:
		case 3:
			name = u8"攻撃段階";
			break;
		case 1:
		case 4:
			name = u8"攻撃頻度段階";
			break;
		case 2:
		case 5:
			name = u8"回避CT段階";
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillShot:
			name = u8"スキル:遠距離攻撃";
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillNova:
			name = u8"スキル:近接攻撃";
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillOrbit:
			name = u8"スキル:衛星攻撃";
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillShotRange:
			name = u8"遠距離 範囲";
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillShotPower:
			name = u8"遠距離 威力";
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillShotCooldown:
			name = u8"遠距離 CT";
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillNovaRange:
			name = u8"近接 範囲";
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillNovaPower:
			name = u8"近接 威力";
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillNovaCooldown:
			name = u8"近接 CT";
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillOrbitRange:
			name = u8"衛星 範囲";
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillOrbitCooldown:
			name = u8"衛星 CT";
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillOrbitCount:
			name = u8"衛星 数";
			break;
		default:
			name = nullptr;
			break;
		}

		if (!name)
		{
			sprintf_s(out, outSize, "%s", u8"ここには何もないようだ");
			return;
		}

		if (upgradeType >= Transfer::RoguelikeUpgrade::UpgradeSkillShot)
		{
			sprintf_s(out, outSize, "%s", name);
			return;
		}

		if (amount <= 0)
		{
			sprintf_s(out, outSize, "%s", u8"ここには何もないようだ");
			return;
		}

		sprintf_s(out, outSize, u8"%s+%d", name, amount);
	}

	void FormatUpgradeDescription(const Transfer& tran, int upgradeType, char* out, size_t outSize)
	{
		if (!out || outSize == 0) return;

		const int difficultyPreset = tran.NormalizeDifficultyPreset(tran.gameplayDebug.difficultyPreset);
		const int amount = tran.GetUpgradeStepForType(upgradeType, difficultyPreset);
		switch (upgradeType)
		{
		case 0:
		case 3:
			sprintf_s(out, outSize, u8"段階テーブルに沿って攻撃性能を%d段階強化", amount);
			break;
		case 1:
		case 4:
			sprintf_s(out, outSize, u8"段階テーブルに沿って攻撃CTを%d段階短縮", amount);
			break;
		case 2:
		case 5:
			sprintf_s(out, outSize, u8"段階テーブルに沿って回避CTを%d段階短縮", amount);
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillShot:
			sprintf_s(out, outSize, u8"向いている方向へ端まで飛ぶ弾を解放");
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillNova:
			sprintf_s(out, outSize, u8"通常攻撃より広い全方向攻撃を解放");
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillOrbit:
			sprintf_s(out, outSize, u8"プレイヤー周囲を周遊する衛星攻撃を解放");
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillShotRange:
			sprintf_s(out, outSize, u8"遠距離攻撃の当たり範囲を最大3倍まで拡大");
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillShotPower:
			sprintf_s(out, outSize, u8"遠距離攻撃の威力倍率を最大2倍まで上昇");
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillShotCooldown:
			sprintf_s(out, outSize, u8"遠距離攻撃のCTを最大4秒短縮");
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillNovaRange:
			sprintf_s(out, outSize, u8"近接攻撃の範囲を最大3倍まで拡大");
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillNovaPower:
			sprintf_s(out, outSize, u8"近接攻撃の威力倍率を最大2倍まで上昇");
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillNovaCooldown:
			sprintf_s(out, outSize, u8"近接攻撃のCTを最大4秒短縮");
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillOrbitRange:
			sprintf_s(out, outSize, u8"衛星の接触範囲を最大3倍まで拡大");
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillOrbitCooldown:
			sprintf_s(out, outSize, u8"衛星スキルのCTを最大4秒短縮");
			break;
		case Transfer::RoguelikeUpgrade::UpgradeSkillOrbitCount:
			sprintf_s(out, outSize, u8"衛星数を最大6まで増加 以降は威力を最大1.5倍");
			break;
		default:
			out[0] = '\0';
			break;
		}
	}

	void DrawCenteredOverlayText(const char* text, ImU32 fillColor, ImU32 borderColor)
	{
		if (!text || text[0] == '\0') return;

		ImGuiViewport* vp = ImGui::GetMainViewport();
		if (!vp) return;
		ImDrawList* dl = ImGui::GetForegroundDrawList(vp);
		if (!dl) return;

		const ImVec2 pad(14.0f, 12.0f);
		const ImVec2 textSize = ImGui::CalcTextSize(text);
		const ImVec2 boxMin(vp->Pos.x + (vp->Size.x - textSize.x) * 0.5f - pad.x,
							vp->Pos.y + (vp->Size.y - textSize.y) * 0.5f - pad.y);
		const ImVec2 boxMax(boxMin.x + textSize.x + pad.x * 2.0f, boxMin.y + textSize.y + pad.y * 2.0f);

		dl->AddRectFilled(boxMin, boxMax, fillColor, 8.0f);
		dl->AddRect(boxMin, boxMax, borderColor, 8.0f);
		dl->AddText(ImVec2(boxMin.x + pad.x, boxMin.y + pad.y), IM_COL32(255, 255, 255, 255), text);
	}

	void FormatUpgradeTableValue(float value, const char* suffix, char* out, size_t outSize)
	{
		if (!out || outSize == 0) return;
		const float roundedInt = std::round(value);
		const float rounded1 = std::round(value * 10.0f) / 10.0f;
		if (std::fabs(value - roundedInt) < 0.001f)
		{
			sprintf_s(out, outSize, "%.0f%s", roundedInt, suffix);
		}
		else if (std::fabs(value - rounded1) < 0.001f)
		{
			sprintf_s(out, outSize, "%.1f%s", rounded1, suffix);
		}
		else
		{
			sprintf_s(out, outSize, "%.2f%s", value, suffix);
		}
	}

	void FormatUpgradePercentValue(float scale, char* out, size_t outSize)
	{
		if (!out || outSize == 0) return;
		const float percent = scale * 100.0f;
		const float roundedInt = std::round(percent);
		const float rounded1 = std::round(percent * 10.0f) / 10.0f;
		if (std::fabs(percent - roundedInt) < 0.001f)
		{
			sprintf_s(out, outSize, "%.0f%%", roundedInt);
		}
		else if (std::fabs(percent - rounded1) < 0.001f)
		{
			sprintf_s(out, outSize, "%.1f%%", rounded1);
		}
		else
		{
			sprintf_s(out, outSize, "%.2f%%", percent);
		}
	}

	void SetupUpgradeMatrixColumns()
	{
		ImGui::TableSetupColumn(u8"名称", ImGuiTableColumnFlags_WidthFixed, 110.0f);
		ImGui::TableSetupColumn(u8"種類", ImGuiTableColumnFlags_WidthFixed, 120.0f);
		for (int level = 1; level <= 10; ++level)
		{
			char header[16]{};
			sprintf_s(header, "Lv%d", level);
			ImGui::TableSetupColumn(header, ImGuiTableColumnFlags_WidthFixed, 72.0f);
		}
		ImGui::TableSetupScrollFreeze(2, 1);
		ImGui::TableHeadersRow();
	}

	template <typename Formatter>
	void DrawUpgradeMatrixRow(const char* name, const char* kind, Formatter formatter)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::TextUnformatted(name);
		ImGui::TableSetColumnIndex(1);
		ImGui::TextUnformatted(kind);
		for (int level = 1; level <= 10; ++level)
		{
			char cell[32]{};
			formatter(level, cell, sizeof(cell));
			ImGui::TableSetColumnIndex(level + 1);
			ImGui::TextUnformatted(cell);
		}
	}

	void DrawStatusUpgradeValueTable(const Transfer& tran)
	{
		const ImGuiTableFlags flags =
			ImGuiTableFlags_Borders |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_SizingFixedFit |
			ImGuiTableFlags_ScrollX |
			ImGuiTableFlags_ScrollY;
		const ImVec2 outerSize(ImGui::GetContentRegionAvail().x, 160.0f);
		if (!ImGui::BeginTable("##status_upgrade_table", 12, flags, outerSize))
		{
			return;
		}

		SetupUpgradeMatrixColumns();
		DrawUpgradeMatrixRow(u8"プレイヤー", u8"攻撃ダメージ", [&](int level, char* out, size_t outSize)
		{
			sprintf_s(out, outSize, "%d", tran.GetPlayerAttackDamageByLevel(level));
		});
		DrawUpgradeMatrixRow(u8"プレイヤー", u8"攻撃CT倍率", [&](int level, char* out, size_t outSize)
		{
			FormatUpgradePercentValue(tran.GetAttackCooldownScaleByLevel(level), out, outSize);
		});
		DrawUpgradeMatrixRow(u8"プレイヤー", u8"回避CT倍率", [&](int level, char* out, size_t outSize)
		{
			FormatUpgradePercentValue(tran.GetEvadeCooldownScaleByLevel(level), out, outSize);
		});

		ImGui::EndTable();
	}

	void DrawSkillUpgradeValueTable(const Transfer& tran)
	{
		const ImGuiTableFlags flags =
			ImGuiTableFlags_Borders |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_SizingFixedFit |
			ImGuiTableFlags_ScrollX |
			ImGuiTableFlags_ScrollY;
		const ImVec2 outerSize(ImGui::GetContentRegionAvail().x, 260.0f);
		if (!ImGui::BeginTable("##skill_upgrade_table", 12, flags, outerSize))
		{
			return;
		}

		SetupUpgradeMatrixColumns();
		DrawUpgradeMatrixRow(u8"近接攻撃", u8"ダメージ倍率", [&](int level, char* out, size_t outSize)
		{
			FormatUpgradePercentValue(tran.GetSkillDamageScaleByLevel(level), out, outSize);
		});
		DrawUpgradeMatrixRow(u8"近接攻撃", u8"攻撃範囲", [&](int level, char* out, size_t outSize)
		{
			FormatUpgradePercentValue(tran.GetSkillRangeScaleByLevel(level), out, outSize);
		});
		DrawUpgradeMatrixRow(u8"近接攻撃", u8"CT短縮", [&](int level, char* out, size_t outSize)
		{
			const float seconds = tran.GetSkillCooldownReductionByLevel(level);
			char value[32]{};
			FormatUpgradeTableValue(seconds, u8"", value, sizeof(value));
			sprintf_s(out, outSize, u8"-%s秒", value);
		});
		DrawUpgradeMatrixRow(u8"遠距離攻撃", u8"ダメージ倍率", [&](int level, char* out, size_t outSize)
		{
			FormatUpgradePercentValue(tran.GetSkillDamageScaleByLevel(level), out, outSize);
		});
		DrawUpgradeMatrixRow(u8"遠距離攻撃", u8"攻撃範囲", [&](int level, char* out, size_t outSize)
		{
			FormatUpgradePercentValue(tran.GetSkillRangeScaleByLevel(level), out, outSize);
		});
		DrawUpgradeMatrixRow(u8"遠距離攻撃", u8"CT短縮", [&](int level, char* out, size_t outSize)
		{
			const float seconds = tran.GetSkillCooldownReductionByLevel(level);
			char value[32]{};
			FormatUpgradeTableValue(seconds, u8"", value, sizeof(value));
			sprintf_s(out, outSize, u8"-%s秒", value);
		});
		DrawUpgradeMatrixRow(u8"衛星攻撃", u8"攻撃範囲", [&](int level, char* out, size_t outSize)
		{
			FormatUpgradePercentValue(tran.GetSkillRangeScaleByLevel(level), out, outSize);
		});
		DrawUpgradeMatrixRow(u8"衛星攻撃", u8"CT短縮", [&](int level, char* out, size_t outSize)
		{
			const float seconds = tran.GetSkillCooldownReductionByLevel(level);
			char value[32]{};
			FormatUpgradeTableValue(seconds, u8"", value, sizeof(value));
			sprintf_s(out, outSize, u8"-%s秒", value);
		});
		DrawUpgradeMatrixRow(u8"衛星攻撃", u8"生成数", [&](int level, char* out, size_t outSize)
		{
			sprintf_s(out, outSize, u8"%d個", tran.GetOrbitCountByLevel(level));
		});
		DrawUpgradeMatrixRow(u8"衛星攻撃", u8"威力倍率", [&](int level, char* out, size_t outSize)
		{
			FormatUpgradePercentValue(tran.GetOrbitDamageScaleByCountLevel(level), out, outSize);
		});

		ImGui::EndTable();
	}

	void DrawUpgradeSelectionOverlay(const Transfer& tran, bool showBossDebugHint)
	{
		if (SceneManager::GetCurrent() != SceneManager::SceneType::SCENE_RESULT) return;
		if (SceneManager::GetResultType() != SceneManager::ResultType::Win) return;
		if (tran.roguelike.selectionPending == 0) return;

		const bool hasAnyOffer =
			(tran.roguelike.offers[0] >= 0) ||
			(tran.roguelike.offers[1] >= 0) ||
			(tran.roguelike.offers[2] >= 0);
		int optionIndex = tran.gameplayDebug.rewardSelectionIndex;
		if (optionIndex < 0) optionIndex = 0;
		const bool isStatusPhase = (tran.roguelike.selectionPhase == Transfer::RoguelikeUpgrade::SelectionStatus);
		const char* overlayTitle = isStatusPhase ? u8"ステータス強化" : u8"スキル獲得 / 強化";
		const char* continueLabel = isStatusPhase ? u8"スキル強化フェーズへ" : u8"結果へ進む";

		char upgradeHud[1024]{};
		if (!hasAnyOffer)
		{
			if (showBossDebugHint)
			{
				sprintf_s(
					upgradeHud,
					u8"%s\n\nなにもない\n\n%s %s\n%s ボス戦へ（デバッグ）\n\n[方向キー / 左スティック] 選択  [A / Enter / Space] 決定",
					overlayTitle,
					(optionIndex == 0) ? u8">" : u8" ",
					continueLabel,
					(optionIndex == 1) ? u8">" : u8" ");
			}
			else
			{
				sprintf_s(
					upgradeHud,
					u8"%s\n\nなにもない\n\n%s %s\n\n[方向キー / 左スティック] 選択  [A / Enter / Space] 決定",
					overlayTitle,
					(optionIndex == 0) ? u8">" : u8" ",
					continueLabel);
			}
		}
		else
		{
			char l0[64]{}, l1[64]{}, l2[64]{};
			char d0[96]{}, d1[96]{}, d2[96]{};
			FormatUpgradeLabel(tran, tran.roguelike.offers[0], l0, sizeof(l0));
			FormatUpgradeLabel(tran, tran.roguelike.offers[1], l1, sizeof(l1));
			FormatUpgradeLabel(tran, tran.roguelike.offers[2], l2, sizeof(l2));
			FormatUpgradeDescription(tran, tran.roguelike.offers[0], d0, sizeof(d0));
			FormatUpgradeDescription(tran, tran.roguelike.offers[1], d1, sizeof(d1));
			FormatUpgradeDescription(tran, tran.roguelike.offers[2], d2, sizeof(d2));

			if (showBossDebugHint)
			{
				sprintf_s(
					upgradeHud,
					u8"%s: 1つ選択\n\n%s %s\n    %s\n%s %s\n    %s\n%s %s\n    %s\n%s ボス戦へ（デバッグ）\n\n[方向キー / 左スティック] 選択  [A / Enter / Space] 決定\n[R / Y] リロール: 残り %d / %d",
					overlayTitle,
					(optionIndex == 0) ? u8">" : u8" ", l0, d0,
					(optionIndex == 1) ? u8">" : u8" ", l1, d1,
					(optionIndex == 2) ? u8">" : u8" ", l2, d2,
					(optionIndex == 3) ? u8">" : u8" ",
					tran.roguelike.rerollRemain,
					tran.roguelike.rerollMaxPerStage);
			}
			else
			{
				sprintf_s(
					upgradeHud,
					u8"%s: 1つ選択\n\n%s %s\n    %s\n%s %s\n    %s\n%s %s\n    %s\n\n[方向キー / 左スティック] 選択  [A / Enter / Space] 決定\n[R / Y] リロール: 残り %d / %d",
					overlayTitle,
					(optionIndex == 0) ? u8">" : u8" ", l0, d0,
					(optionIndex == 1) ? u8">" : u8" ", l1, d1,
					(optionIndex == 2) ? u8">" : u8" ", l2, d2,
					tran.roguelike.rerollRemain,
					tran.roguelike.rerollMaxPerStage);
			}
		}

		DrawCenteredOverlayText(upgradeHud, IM_COL32(0, 0, 0, 210), IM_COL32(255, 255, 255, 160));
	}

	bool IsEngineEditorScene(SceneManager::SceneType sceneType)
	{
		return sceneType == SceneManager::SceneType::SCENE_ENGINE_EDITOR;
	}

	SceneCastleEditor* GetCastleEditorScene()
	{
		if (!IsEngineEditorScene(SceneManager::GetCurrent())) return nullptr;
		return dynamic_cast<SceneCastleEditor*>(SceneManager::GetScene());
	}

	RenderTarget* g_pEngineEditorRT = nullptr;
	DepthStencil* g_pEngineEditorDS = nullptr;
	UINT g_engineEditorRTWidth = 0;
	UINT g_engineEditorRTHeight = 0;
	UINT g_engineEditorRequestWidth = 640;
	UINT g_engineEditorRequestHeight = 360;

	void ReleaseEngineEditorRenderTargets()
	{
		SAFE_DELETE(g_pEngineEditorDS);
		SAFE_DELETE(g_pEngineEditorRT);
		g_engineEditorRTWidth = 0;
		g_engineEditorRTHeight = 0;
	}

	bool EnsureEngineEditorRenderTarget(UINT width, UINT height)
	{
		if (width < 16 || height < 16) return false;
		if (g_pEngineEditorRT &&
			g_pEngineEditorDS &&
			g_engineEditorRTWidth == width &&
			g_engineEditorRTHeight == height)
		{
			return true;
		}

		ReleaseEngineEditorRenderTargets();
		g_pEngineEditorRT = new RenderTarget();
		if (FAILED(g_pEngineEditorRT->Create(DXGI_FORMAT_R8G8B8A8_UNORM, width, height)))
		{
			ReleaseEngineEditorRenderTargets();
			return false;
		}

		g_pEngineEditorDS = new DepthStencil();
		if (FAILED(g_pEngineEditorDS->Create(width, height, false)))
		{
			ReleaseEngineEditorRenderTargets();
			return false;
		}

		g_engineEditorRTWidth = width;
		g_engineEditorRTHeight = height;
		return true;
	}

	void DrawCastleEditorPaletteWindow(SceneCastleEditor* editor)
	{
		if (!editor) return;
		if (!ImGui::Begin(u8"パレット"))
		{
			ImGui::End();
			return;
		}

		const int selectedAssetIndex = editor->GetSelectedAssetIndex();
		const SceneCastleEditor::ToolMode toolMode = editor->GetToolMode();
		ImGui::TextDisabled(u8"Build Assets");
		if (ImGui::BeginChild("##asset_grid", ImVec2(0.0f, 170.0f), true))
		{
			const int columnCount = 8;
			if (ImGui::BeginTable("AssetGrid", columnCount, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_BordersInnerV))
			{
				int tileIndex = 0;
				const float contentWidth = ImGui::GetContentRegionAvail().x;
				const float spacingX = ImGui::GetStyle().ItemSpacing.x;
				float tileSize = (contentWidth - spacingX * static_cast<float>(columnCount - 1)) / static_cast<float>(columnCount);
				if (tileSize < 56.0f) tileSize = 56.0f;
				const float innerPadding = 6.0f;
				const auto drawTile = [&](const char* label, bool selected, int assetIndex)
				{
					ImGui::TableNextColumn();
					ImGui::PushID(tileIndex++);
					const ImVec2 pos = ImGui::GetCursorScreenPos();
					ImGui::InvisibleButton("##asset_tile", ImVec2(tileSize, tileSize));
					const bool hovered = ImGui::IsItemHovered();
					const bool clicked = ImGui::IsItemClicked();
					ImDrawList* drawList = ImGui::GetWindowDrawList();
					const ImVec2 min = pos;
					const ImVec2 max(pos.x + tileSize, pos.y + tileSize);
					const ImU32 bgColor = selected
						? IM_COL32(54, 102, 156, 255)
						: hovered ? IM_COL32(48, 56, 68, 255) : IM_COL32(36, 42, 52, 255);
					const ImU32 borderColor = selected ? IM_COL32(130, 200, 255, 255) : IM_COL32(84, 92, 108, 255);
					drawList->AddRectFilled(min, max, bgColor, 8.0f);
					drawList->AddRect(min, max, borderColor, 8.0f, 0, selected ? 2.0f : 1.0f);

					const ImVec2 thumbMin(min.x + innerPadding, min.y + innerPadding);
					const ImVec2 thumbMax(max.x - innerPadding, max.y - 24.0f);
					if (assetIndex >= 0)
					{
						void* textureId = editor->GetAssetThumbnailTextureId(assetIndex, static_cast<UINT>(thumbMax.x - thumbMin.x));
						if (textureId)
						{
							drawList->AddImage(textureId, thumbMin, thumbMax);
						}
						else
						{
							drawList->AddRectFilled(thumbMin, thumbMax, IM_COL32(28, 32, 40, 255), 6.0f);
						}
					}
					else
					{
						drawList->AddRectFilled(thumbMin, thumbMax, IM_COL32(28, 32, 40, 255), 6.0f);
						const ImVec2 center((thumbMin.x + thumbMax.x) * 0.5f, (thumbMin.y + thumbMax.y) * 0.5f);
						drawList->AddCircle(center, (thumbMax.x - thumbMin.x) * 0.18f, IM_COL32(220, 228, 240, 255), 32, 2.0f);
						drawList->AddLine(ImVec2(center.x + 10.0f, center.y + 10.0f), ImVec2(center.x + 20.0f, center.y + 20.0f), IM_COL32(220, 228, 240, 255), 2.0f);
					}

					const ImVec2 textSize = ImGui::CalcTextSize(label);
					drawList->AddText(
						ImVec2(min.x + (tileSize - textSize.x) * 0.5f, max.y - 18.0f),
						IM_COL32(236, 240, 246, 255),
						label);
					if (clicked)
					{
						if (assetIndex < 0)
						{
							editor->SetToolMode(SceneCastleEditor::ToolMode::SelectSingle);
						}
						else
						{
							editor->SetSelectedAssetIndex(assetIndex);
						}
					}
					ImGui::PopID();
				};

				drawTile(
					u8"Select",
					toolMode == SceneCastleEditor::ToolMode::SelectSingle ||
					toolMode == SceneCastleEditor::ToolMode::SelectFill,
					-1);
				for (int i = 0; i < editor->GetAssetCount(); ++i)
				{
					const SceneCastleEditor::AssetInfo* asset = editor->GetAssetInfo(i);
					if (!asset) continue;
					drawTile(
						asset->name,
						toolMode != SceneCastleEditor::ToolMode::SelectSingle &&
						toolMode != SceneCastleEditor::ToolMode::SelectFill &&
						selectedAssetIndex == i,
						i);
				}

				while (tileIndex % columnCount != 0)
				{
					ImGui::TableNextColumn();
					tileIndex++;
				}

				ImGui::EndTable();
			}
		}
		ImGui::EndChild();

		if (ImGui::Button(u8"回転 -90"))
		{
			editor->RotatePreview(-1);
		}
		ImGui::SameLine();
		if (ImGui::Button(u8"回転 +90"))
		{
			editor->RotatePreview(1);
		}

		ImGui::SeparatorText(u8"ツール");
		if (ImGui::RadioButton(u8"Select", toolMode == SceneCastleEditor::ToolMode::SelectSingle))
		{
			editor->SetToolMode(SceneCastleEditor::ToolMode::SelectSingle);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton(u8"Select Fill", toolMode == SceneCastleEditor::ToolMode::SelectFill))
		{
			editor->SetToolMode(SceneCastleEditor::ToolMode::SelectFill);
		}
		if (selectedAssetIndex >= 0)
		{
			ImGui::SameLine();
			if (ImGui::RadioButton(u8"Place", toolMode == SceneCastleEditor::ToolMode::PlaceSingle))
			{
				editor->SetToolMode(SceneCastleEditor::ToolMode::PlaceSingle);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton(u8"Paint", toolMode == SceneCastleEditor::ToolMode::PlacePaint))
			{
				editor->SetToolMode(SceneCastleEditor::ToolMode::PlacePaint);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton(u8"Fill", toolMode == SceneCastleEditor::ToolMode::PlaceFill))
			{
				editor->SetToolMode(SceneCastleEditor::ToolMode::PlaceFill);
			}
		}

		ImGui::SeparatorText(u8"操作");
		if (toolMode == SceneCastleEditor::ToolMode::SelectSingle)
		{
			ImGui::TextUnformatted(u8"左クリック: 選択 / Ctrl+クリック: 追加解除 / Shift+クリック: 追加");
			if (editor->IsSelectFillStartActive())
			{
				ImGui::TextDisabled(u8"Select Fill の始点設定が残っています。");
			}
		}
		else if (toolMode == SceneCastleEditor::ToolMode::SelectFill)
		{
			ImGui::TextUnformatted(u8"LMB: 始点選択 -> LMB: 終点選択で3D範囲選択");
			ImGui::TextDisabled(u8"Ctrlを押しながら確定すると既存選択へ追加");
			ImGui::TextDisabled(u8"空セルも指定可能 / Shift押下中はアクティブレイヤーへスナップ");
			if (editor->IsSelectFillStartActive())
			{
				ImGui::TextDisabled(u8"始点設定済み / 終点までの直方体を選択");
			}
		}
		else if (toolMode == SceneCastleEditor::ToolMode::PlacePaint)
		{
			ImGui::TextUnformatted(u8"LMB Drag: 連続配置");
		}
		else if (toolMode == SceneCastleEditor::ToolMode::PlaceFill)
		{
			ImGui::TextUnformatted(u8"LMB: 始点選択 -> LMB: 終点選択で範囲配置");
			ImGui::TextDisabled(u8"Shift+Wheel: 始点からの高さオフセット変更");
			ImGui::TextDisabled(u8"Shift中も水平面配置は可能 / 高さ差をつけると縦面配置");
			if (editor->IsFillStartActive())
			{
				ImGui::TextDisabled(u8"始点設定済み / 終点は同一平面上で指定");
			}
		}
		else
		{
			ImGui::TextUnformatted(u8"左クリック: 面の隣に配置");
		}
		ImGui::TextUnformatted(u8"右ドラッグ: 回転");
		ImGui::TextUnformatted(u8"中ドラッグ: 平行移動");
		ImGui::TextUnformatted(u8"ホイール: ズーム");
		ImGui::TextUnformatted(u8"R / Shift+R: 選択中オブジェクト回転");
		ImGui::TextUnformatted(u8"Delete: 選択中オブジェクト削除");
		ImGui::TextUnformatted(u8"Ctrl+Z / Ctrl+Y: Undo / Redo");
		if (toolMode == SceneCastleEditor::ToolMode::SelectSingle)
		{
			ImGui::TextDisabled(u8"現在: Select ツール");
		}
		else if (toolMode == SceneCastleEditor::ToolMode::SelectFill)
		{
			ImGui::TextDisabled(u8"現在: Select Fill ツール");
		}
		else if (toolMode == SceneCastleEditor::ToolMode::PlacePaint)
		{
			const SceneCastleEditor::AssetInfo* asset = editor->GetAssetInfo(selectedAssetIndex);
			ImGui::TextDisabled(u8"現在: Paint (%s)", asset ? asset->name : u8"Unknown");
		}
		else if (toolMode == SceneCastleEditor::ToolMode::PlaceFill)
		{
			const SceneCastleEditor::AssetInfo* asset = editor->GetAssetInfo(selectedAssetIndex);
			ImGui::TextDisabled(u8"現在: Fill (%s)", asset ? asset->name : u8"Unknown");
		}
		else
		{
			const SceneCastleEditor::AssetInfo* asset = editor->GetAssetInfo(selectedAssetIndex);
			ImGui::TextDisabled(u8"現在: Place (%s)", asset ? asset->name : u8"Unknown");
		}

		ImGui::End();
	}

	void DrawCastleEditorHierarchyWindow(SceneCastleEditor* editor)
	{
		if (!editor) return;
		if (!ImGui::Begin(u8"配置一覧"))
		{
			ImGui::End();
			return;
		}

		for (int i = 0; i < editor->GetPlacementCount(); ++i)
		{
			const SceneCastleEditor::PlacementInfo* placement = editor->GetPlacement(i);
			if (!placement) continue;
			const SceneCastleEditor::AssetInfo* asset = editor->GetAssetInfo(placement->assetIndex);
			char label[128];
			sprintf_s(
				label,
				"%s (%d, %d, %d)",
				asset ? asset->name : "Unknown",
				placement->gridX,
				placement->gridY,
				placement->gridZ);
			const bool selected = editor->IsPlacementSelected(i);
			if (ImGui::Selectable(label, selected))
			{
				const ImGuiIO& io = ImGui::GetIO();
				if (io.KeyCtrl)
				{
					editor->ToggleSelectedPlacementIndex(i);
				}
				else if (io.KeyShift)
				{
					editor->AddSelectedPlacementIndex(i);
				}
				else
				{
					editor->SetSelectedPlacementIndex(i);
				}
			}
		}

		if (ImGui::Button(u8"選択解除"))
		{
			editor->ClearSelection();
		}

		ImGui::End();
	}

	void DrawCastleEditorInspectorWindow(SceneCastleEditor* editor)
	{
		if (!editor) return;
		static int castleSaveStatus = 0;
		if (!ImGui::Begin(u8"インスペクタ"))
		{
			ImGui::End();
			return;
		}

		const int selectedCount = editor->GetSelectedPlacementCount();
		const int selectedIndex = (selectedCount == 1) ? editor->GetSelectedPlacementIndex() : -1;
		const bool canUndo = editor->CanUndo();
		const bool canRedo = editor->CanRedo();
		if (!canUndo) ImGui::BeginDisabled();
		if (ImGui::Button(u8"Undo"))
		{
			editor->Undo();
		}
		if (!canUndo) ImGui::EndDisabled();
		ImGui::SameLine();
		if (!canRedo) ImGui::BeginDisabled();
		if (ImGui::Button(u8"Redo"))
		{
			editor->Redo();
		}
		if (!canRedo) ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button(u8"保存"))
		{
			castleSaveStatus = editor->SaveCastleData() ? 1 : 2;
		}
		ImGui::SameLine();
		if (ImGui::Button(u8"読込"))
		{
			castleSaveStatus = editor->LoadCastleData() ? 3 : 4;
		}
		ImGui::TextDisabled(u8"保存先: %s", CastleSaveData::GetDefaultPath());
		switch (castleSaveStatus)
		{
		case 1:
			ImGui::TextColored(ImVec4(0.45f, 0.82f, 0.55f, 1.0f), u8"保存成功");
			break;
		case 2:
			ImGui::TextColored(ImVec4(0.92f, 0.38f, 0.38f, 1.0f), u8"保存失敗");
			break;
		case 3:
			ImGui::TextColored(ImVec4(0.45f, 0.82f, 0.55f, 1.0f), u8"読込成功");
			break;
		case 4:
			ImGui::TextColored(ImVec4(0.92f, 0.38f, 0.38f, 1.0f), u8"読込失敗");
			break;
		default:
			break;
		}
		ImGui::Separator();

		if (selectedCount > 1)
		{
			ImGui::Text(u8"%d件選択中", selectedCount);
			ImGui::TextDisabled(u8"位置や回転の編集は1件選択時のみ可能です。");
			if (ImGui::Button(u8"選択中を一括削除"))
			{
				editor->DeleteSelectedPlacement();
			}
		}
		else if (selectedIndex >= 0)
		{
			const SceneCastleEditor::PlacementInfo* placement = editor->GetPlacement(selectedIndex);
			const SceneCastleEditor::AssetInfo* asset = placement ? editor->GetAssetInfo(placement->assetIndex) : nullptr;
			if (placement)
			{
				ImGui::Text(u8"選択中: %s", asset ? asset->name : u8"Unknown");

				int gridX = placement->gridX;
				int gridY = placement->gridY;
				int gridZ = placement->gridZ;
				int rotation = placement->rotationQuarterTurns;

				bool changed = false;
				changed |= ImGui::InputInt("Grid X", &gridX);
				changed |= ImGui::InputInt("Grid Y", &gridY);
				changed |= ImGui::InputInt("Grid Z", &gridZ);
				changed |= ImGui::InputInt(u8"回転(90度単位)", &rotation);
				if (changed)
				{
					editor->UpdateSelectedPlacement(gridX, gridY, gridZ, rotation);
				}

				if (ImGui::Button(u8"削除"))
				{
					editor->DeleteSelectedPlacement();
				}
			}
		}
		else
		{
			ImGui::TextDisabled(u8"配置済みオブジェクトを選択すると詳細を編集できます。");
			if (editor->HasPreview())
			{
				ImGui::SeparatorText(u8"プレビュー");
				ImGui::TextUnformatted(editor->CanPlacePreview() ? u8"配置可能" : u8"配置不可");
			}
		}

		const int modelViewAssetIndex =
			(editor->GetSelectedAssetIndex() >= 0)
			? editor->GetSelectedAssetIndex()
			: ((selectedIndex >= 0 && editor->GetPlacement(selectedIndex))
				? editor->GetPlacement(selectedIndex)->assetIndex
				: -1);

		ImGui::SeparatorText(u8"ModelView");
		if (modelViewAssetIndex >= 0)
		{
			const float viewSize = ImGui::GetContentRegionAvail().x;
			const float clampedSize = (viewSize < 180.0f) ? 180.0f : viewSize;
			const ImVec2 imagePos = ImGui::GetCursorScreenPos();
			void* textureId = editor->GetModelViewTextureId(static_cast<unsigned int>(clampedSize));
			if (textureId)
			{
				ImGui::Image(textureId, ImVec2(clampedSize, clampedSize));
			}
			else
			{
				ImGui::InvisibleButton("##model_view_dummy", ImVec2(clampedSize, clampedSize));
			}

			const bool hovered = ImGui::IsItemHovered();
			ImGuiIO& io = ImGui::GetIO();
			editor->HandleModelViewInput(
				hovered,
				hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Right),
				io.MouseDelta.x,
				io.MouseDelta.y,
				hovered ? io.MouseWheel : 0.0f);

			ImGui::GetWindowDrawList()->AddText(
				ImVec2(imagePos.x + 10.0f, imagePos.y + 10.0f),
				IM_COL32(240, 245, 255, 255),
				u8"RMB: Orbit  Wheel: Zoom");
			if (ImGui::Button(u8"ModelViewリセット"))
			{
				editor->ResetModelViewCamera();
			}
		}
		else
		{
			ImGui::TextDisabled(u8"アセットか配置オブジェクトを選択すると表示されます。");
		}

		ImGui::End();
	}

	void DrawCastleEditorCameraWindow(SceneCastleEditor* editor)
	{
		if (!editor) return;
		if (!ImGui::Begin(u8"カメラ"))
		{
			ImGui::End();
			return;
		}

		DirectX::XMFLOAT3 eye = editor->GetCameraEye();
		DirectX::XMFLOAT3 look = editor->GetCameraLook();
		int activeLayer = editor->GetActiveLayer();
		bool changed = false;
		changed |= ImGui::DragFloat3("Eye", reinterpret_cast<float*>(&eye), 0.05f);
		changed |= ImGui::DragFloat3("Look", reinterpret_cast<float*>(&look), 0.05f);
		if (changed)
		{
			editor->SetCameraEye(eye);
			editor->SetCameraLook(look);
		}

		if (ImGui::Button(u8"カメラをリセット"))
		{
			editor->ResetCamera();
		}
		if (ImGui::InputInt(u8"アクティブレイヤー", &activeLayer))
		{
			editor->SetActiveLayer(activeLayer);
		}
		ImGui::TextDisabled(u8"Shift+Wheel: アクティブレイヤー変更");

		ImGui::End();
	}

	void DrawEngineEditorSceneViewWindow(SceneCastleEditor* editor)
	{
		if (!editor) return;
		if (!ImGui::Begin(u8"シーンビュー"))
		{
			ImGui::End();
			return;
		}

		ImVec2 area = ImGui::GetContentRegionAvail();
		if (area.x < 320.0f) area.x = 320.0f;
		if (area.y < 220.0f) area.y = 220.0f;
		g_engineEditorRequestWidth = static_cast<UINT>(area.x);
		g_engineEditorRequestHeight = static_cast<UINT>(area.y);

		const bool targetReady = EnsureEngineEditorRenderTarget(g_engineEditorRequestWidth, g_engineEditorRequestHeight);
		ImVec2 imageTopLeft = ImGui::GetCursorScreenPos();
		bool imageHovered = false;
		if (targetReady && g_pEngineEditorRT && g_pEngineEditorRT->GetResource())
		{
			ImGui::Image(reinterpret_cast<ImTextureID>(g_pEngineEditorRT->GetResource()), area);
			imageHovered = ImGui::IsItemHovered();
		}
		else
		{
			ImGui::InvisibleButton("##castle_scene_view_dummy", area);
			imageHovered = ImGui::IsItemHovered();
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const ImVec2 bottomRight(imageTopLeft.x + area.x, imageTopLeft.y + area.y);
			drawList->AddRectFilled(imageTopLeft, bottomRight, IM_COL32(16, 24, 32, 255), 4.0f);
			drawList->AddRect(imageTopLeft, bottomRight, IM_COL32(120, 160, 200, 255), 4.0f);
		}

		ImGuiIO& io = ImGui::GetIO();
		const float localMouseX = io.MousePos.x - imageTopLeft.x;
		const float localMouseY = io.MousePos.y - imageTopLeft.y;
		editor->HandleSceneViewInput(
			localMouseX,
			localMouseY,
			area.x,
			area.y,
			imageHovered,
			imageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left),
			imageHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left),
			io.KeyCtrl,
			io.KeyShift,
			imageHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Right),
			imageHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle),
			io.MouseDelta.x,
			io.MouseDelta.y,
			imageHovered ? io.MouseWheel : 0.0f);

		const ImVec2 overlayPos(imageTopLeft.x + 12.0f, imageTopLeft.y + 12.0f);
		ImGui::GetWindowDrawList()->AddText(
			overlayPos,
			IM_COL32(240, 245, 255, 255),
			(editor->GetToolMode() == SceneCastleEditor::ToolMode::SelectSingle)
				? u8"LMB: Select  Ctrl/Shift+LMB: Multi  RMB: Orbit  MMB: Pan  Wheel: Zoom"
				: (editor->GetToolMode() == SceneCastleEditor::ToolMode::SelectFill)
					? u8"LMB: Start/End Select Box  Ctrl+Confirm: Add  Shift: Layer Snap  Shift+Wheel: Layer  RMB: Orbit  MMB: Pan"
				: (editor->GetToolMode() == SceneCastleEditor::ToolMode::PlacePaint)
					? u8"LMB Drag: Paint  RMB: Orbit  MMB: Pan  Wheel: Zoom"
					: (editor->GetToolMode() == SceneCastleEditor::ToolMode::PlaceFill)
						? u8"LMB: Start/End Fill  Shift+Wheel: Height  RMB: Orbit  MMB: Pan  Wheel: Zoom"
					: u8"LMB: Place  RMB: Orbit  MMB: Pan  Wheel: Zoom");

		ImGui::End();
	}

	void DrawEngineEditorWindows()
	{
		SceneCastleEditor* editor = GetCastleEditorScene();
		if (!editor) return;

		DrawCastleEditorPaletteWindow(editor);
		DrawCastleEditorHierarchyWindow(editor);
		DrawCastleEditorInspectorWindow(editor);
		DrawCastleEditorCameraWindow(editor);
		DrawEngineEditorSceneViewWindow(editor);

		ImGuiIO& io = ImGui::GetIO();
		if (!io.WantTextInput && io.KeyCtrl)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_Z, false))
			{
				if (io.KeyShift)
				{
					editor->Redo();
				}
				else
				{
					editor->Undo();
				}
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_Y, false))
			{
				editor->Redo();
			}
		}

		if (!io.WantTextInput &&
			editor->GetSelectedPlacementCount() == 1 &&
			ImGui::IsKeyPressed(ImGuiKey_R, false))
		{
			const SceneCastleEditor::PlacementInfo* placement = editor->GetPlacement(editor->GetSelectedPlacementIndex());
			if (placement)
			{
				const int rotationDelta = io.KeyShift ? -1 : 1;
				editor->UpdateSelectedPlacement(
					placement->gridX,
					placement->gridY,
					placement->gridZ,
					placement->rotationQuarterTurns + rotationDelta);
			}
		}

		if (editor->GetSelectedPlacementCount() > 0 &&
			ImGui::IsKeyPressed(ImGuiKey_Delete, false) &&
			!io.WantTextInput)
		{
			editor->DeleteSelectedPlacement();
		}
	}

	void DrawSceneToEngineEditorRenderTarget()
	{
		if (!IsEngineEditorScene(SceneManager::GetCurrent())) return;
		if (!EnsureEngineEditorRenderTarget(g_engineEditorRequestWidth, g_engineEditorRequestHeight)) return;
		if (!g_pEngineEditorRT || !g_pEngineEditorDS) return;

		RenderTarget* previewTarget[1] = { g_pEngineEditorRT };
		SetRenderTargets(1, previewTarget, g_pEngineEditorDS);
		const float clearColor[4] = { 0.08f, 0.10f, 0.12f, 1.0f };
		g_pEngineEditorRT->Clear(clearColor);
		g_pEngineEditorDS->Clear();

		SceneManager::Draw();

		RenderTarget* defaultTarget[1] = { GetDefaultRTV() };
		SetRenderTargets(1, defaultTarget, GetDefaultDSV());
	}

}

HRESULT Init(HWND hWnd, UINT width, UINT height)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	HRESULT hr;
	hr = InitSound();
	if (FAILED(hr)) { return hr; }

	// DirectX初期化
	hr = InitDirectX(hWnd, width, height, false);
	if (FAILED(hr)) { UninitSound(); return hr; }

	std::srand(static_cast<unsigned int>(std::time(NULL)));
	{
		TRAN_INS;
		tran.LoadGameplayTuning();
	}

	// 他機能初期化
	Geometory::Init();
	Sprite::Init();
	InitInput();
	ShaderList::Init();

	// シーン
	SceneManager::Init();

	return hr;
}

void Uninit()
{
	double t0 = NowMS();
	DebugLog("App shutdown begin\n");
	{
		TRAN_INS;
		tran.SaveGameplayTuning();
	}

	SceneManager::Uninit();

	ShaderList::Uninit();
	UninitInput();
	Sprite::Uninit();
	Geometory::Uninit();
	ReleaseEngineEditorRenderTargets();
	UninitSound();
	UninitDirectX();

	DebugLog("App shutdown end : %.2f ms\n", NowMS() - t0);
}

void Update()
{
	UpdateInput();
	{
		TRAN_INS;
		SetMasterVolume(tran.gameplay.volumeMaster);
		SetBgmVolume(tran.gameplay.volumeBgm);
		SetSeVolume(tran.gameplay.volumeSe);
	}
	UpdateSound();
	SceneManager::Update();
}

void Draw()
{
	BeginDrawDirectX();

#ifdef _DEBUG
	TRAN_INS;
	auto formatRunTime = [](float sec, char* out, size_t outSize)
	{
		float safeSec = sec;
		if (safeSec < 0.0f) safeSec = 0.0f;
		const int totalMin = static_cast<int>(safeSec / 60.0f);
		const float remain = safeSec - static_cast<float>(totalMin) * 60.0f;
		int totalSec = static_cast<int>(remain);
		int centi = static_cast<int>((remain - static_cast<float>(totalSec)) * 100.0f + 0.5f);
		if (centi >= 100)
		{
			centi = 0;
			++totalSec;
		}
		if (totalSec >= 60)
		{
			totalSec -= 60;
		}
		sprintf_s(out, outSize, "%02d:%02d.%02d", totalMin, totalSec, centi);
	};

	// Docking用のルート（上下左右の吸着・分割/再結合）
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground;
		ImGui::Begin("DockSpaceRoot", nullptr, window_flags);
		ImGui::PopStyleVar(2);
		ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		ImGui::End();
	}

	// ImGuiの描画
	static bool show_main_window = false;

	if(IsKeyTrigger(VK_TAB))show_main_window = !show_main_window;

	using namespace ImGui;
	using namespace std;
	if (show_main_window)
	{
		Begin(u8"メイン設定",&show_main_window);

		string sceneTxt;
		switch (SceneManager::GetCurrent())
		{
		case SceneManager::SceneType::SCENE_TITLE:
			sceneTxt = u8"タイトル";
			break;
		case SceneManager::SceneType::SCENE_GAME:
			sceneTxt = u8"ゲーム";
			break;
		case SceneManager::SceneType::SCENE_RESULT:
			sceneTxt = u8"リザルト";
			break;
		case SceneManager::SceneType::SCENE_ENGINE_EDITOR:
			sceneTxt = u8"城エディタ";
			break;
		default:
			sceneTxt = u8"不明";
			break;
		}
		sceneTxt = u8"現在シーン: " + sceneTxt;
		ImGui::Text(sceneTxt.c_str());

		if (BeginTabBar("TabBar"))
		{
			if (BeginTabItem(u8"プレイヤー"))
			{
				DragFloat3(u8"位置", reinterpret_cast<float*>(&tran.player.pos), 0.05f);
				DragFloat3(u8"サイズ", reinterpret_cast<float*>(&tran.player.size), 0.05f);
				DragFloat3(u8"速度", reinterpret_cast<float*>(&tran.player.velocity), 0.05f);
				DragFloat(u8"移動速度", &tran.player.moveSpeed, 0.01f, 0.0f, 10.0f);
				DragFloat(u8"回避距離", &tran.player.dashDistance, 0.05f, 0.0f, 10.0f);
				DragFloat(u8"回避CT", &tran.player.dashCooldown, 0.01f, 0.0f, 5.0f);
				DragFloat(u8"回避時間", &tran.player.dashDuration, 0.01f, 0.01f, 1.0f);
				DragFloat(u8"ステージサイズ", &tran.player.stageSize, 0.1f, 1.0f, 20.0f);
				DragFloat(u8"床タイルサイズ", &tran.gameplay.groundTileSize, 0.05f, 0.5f, 10.0f);

				ImGui::SeparatorText(u8"プレイヤー攻撃");
				ImGui::TextDisabled(u8"初期値: 準備0.04 / 有効0.12 / 後隙0.10 / CT0.24 / Skill1CT4.0 / Skill2CT9.0");
				DragFloat(u8"攻撃準備", &tran.gameplay.attackWindup, 0.005f, 0.0f, 1.0f);
				DragFloat(u8"攻撃有効", &tran.gameplay.attackDuration, 0.005f, 0.01f, 1.0f);
				DragFloat(u8"攻撃後隙", &tran.gameplay.attackRecovery, 0.005f, 0.0f, 1.0f);
				DragFloat(u8"攻撃CT", &tran.gameplay.attackCooldown, 0.005f, 0.0f, 2.0f);
				DragFloat(u8"スキル1 CT", &tran.gameplay.skill1Cooldown, 0.05f, 0.0f, 30.0f);
				DragFloat(u8"スキル2 CT", &tran.gameplay.skill2Cooldown, 0.05f, 0.0f, 30.0f);
				ImGui::TextDisabled(u8"初期値: 3体ヒット / 時間0.18 / 強さ0.20");
				DragInt(u8"画面揺れ発火ヒット数", &tran.gameplay.screenShakeHitThreshold, 1.0f, 1, 16);
				DragFloat(u8"画面揺れ時間", &tran.gameplay.screenShakeDuration, 0.01f, 0.0f, 1.0f);
				DragFloat(u8"画面揺れ強さ", &tran.gameplay.screenShakeAmplitude, 0.01f, 0.0f, 1.0f);
				DragFloat(u8"薙ぎ角度", &tran.gameplay.attackSweepDegrees, 1.0f, 10.0f, 240.0f);
				DragFloat(u8"攻撃半径倍率", &tran.gameplay.attackSweepRadiusScale, 0.01f, 0.1f, 3.0f);
				DragFloat(u8"攻撃幅倍率", &tran.gameplay.attackWidthScale, 0.01f, 0.1f, 3.0f);
				DragFloat(u8"攻撃奥行倍率", &tran.gameplay.attackDepthScale, 0.01f, 0.1f, 3.0f);
				DragFloat(u8"プレイヤー攻撃ヒットストップ", &tran.gameplay.attackHitStop, 0.001f, 0.0f, 0.20f);
				DragFloat(u8"ノックバック", &tran.gameplay.attackKnockback, 0.01f, 0.0f, 3.0f);
				DragFloat(u8"ヒット発光", &tran.gameplay.attackHitFlash, 0.005f, 0.0f, 0.50f);
				ImGui::TextDisabled(u8"初期値: 軌跡間隔0.02 / 軌跡残存0.16 / 軌跡倍率0.75");
				DragFloat(u8"攻撃軌跡間隔", &tran.gameplay.attackTrailInterval, 0.002f, 0.0f, 0.20f);
				DragFloat(u8"攻撃軌跡残存", &tran.gameplay.attackTrailLife, 0.005f, 0.0f, 0.50f);
				DragFloat(u8"攻撃軌跡倍率", &tran.gameplay.attackTrailScale, 0.01f, 0.1f, 3.0f);

				ImGui::SeparatorText(u8"進行方向マーカー");
				ImGui::TextDisabled(u8"初期値: 通常0.92 / 被り時0.45");
				DragFloat(u8"マーカー透明度(通常)", &tran.gameplay.directionMarkerAlpha, 0.01f, 0.0f, 1.0f);
				DragFloat(u8"マーカー透明度(被り時)", &tran.gameplay.directionMarkerOverlapAlpha, 0.01f, 0.0f, 1.0f);

				ColorEdit4(u8"色", reinterpret_cast<float*>(&tran.player.color));
				EndTabItem();
			}
			if (BeginTabItem(u8"敵・ボス"))
			{
				ImGui::TextDisabled(u8"※ 難易度プリセット適用時に敵の一部設定は上書きされる");

				ImGui::SeparatorText(u8"敵の基本");
				ImGui::TextDisabled(u8"初期値: 敵数3 / 予兆0.55 / CT1.00 / 射程最小0.80 / 射程倍率1.35 / ダメージ1.0");
				DragInt(u8"敵数(基準)", &tran.gameplay.enemyCount, 1.0f, 0, 16);
				DragFloat(u8"敵予兆時間", &tran.gameplay.enemyAttackWindup, 0.01f, 0.0f, 3.0f);
				DragFloat(u8"敵攻撃CT", &tran.gameplay.enemyAttackCooldown, 0.01f, 0.0f, 3.0f);
				DragFloat(u8"敵射程最小", &tran.gameplay.enemyAttackRangeMin, 0.01f, 0.1f, 5.0f);
				DragFloat(u8"敵射程倍率", &tran.gameplay.enemyAttackRangeScale, 0.01f, 0.1f, 5.0f);
				DragFloat(u8"敵ダメージ", &tran.gameplay.enemyAttackDamage, 0.1f, 0.0f, 20.0f);
				DragFloat(u8"敵移動速度", &tran.gameplay.enemyMoveSpeed, 0.01f, 0.1f, 8.0f);
				DragFloat(u8"Wave毎 速度加算", &tran.gameplay.waveEnemyMoveSpeedAdd, 0.01f, 0.0f, 2.0f);
				DragFloat(u8"Wave毎 ダメ倍率加算", &tran.gameplay.waveEnemyAttackDamageScalePerWave, 0.01f, 0.0f, 3.0f);

				ImGui::SeparatorText(u8"遠距離型の敵弾");
				ImGui::TextDisabled(u8"初期値: 速度6.50 / 寿命1.40 / 半径0.22 / ダメ倍率0.85");
				DragFloat(u8"敵弾速度", &tran.gameplay.enemyProjectileSpeed, 0.05f, 0.1f, 20.0f);
				DragFloat(u8"敵弾寿命", &tran.gameplay.enemyProjectileLife, 0.01f, 0.05f, 5.0f);
				DragFloat(u8"敵弾半径", &tran.gameplay.enemyProjectileRadius, 0.01f, 0.05f, 2.0f);
				DragFloat(u8"敵弾ダメ倍率", &tran.gameplay.enemyProjectileDamageScale, 0.01f, 0.0f, 3.0f);

				ImGui::SeparatorText(u8"敵の分離行動");
				ImGui::TextDisabled(u8"初期値: 半径1.10 / 重み0.80 / 最大オフセット0.80");
				DragFloat(u8"分離半径", &tran.gameplay.enemySeparationRadius, 0.01f, 0.0f, 5.0f);
				DragFloat(u8"分離重み", &tran.gameplay.enemySeparationWeight, 0.01f, 0.0f, 3.0f);
				DragFloat(u8"分離最大オフセット", &tran.gameplay.enemySeparationMaxOffset, 0.01f, 0.0f, 3.0f);

				ImGui::SeparatorText(u8"敵スポーン");
				ImGui::TextDisabled(u8"初期値: リング0.35 / ぶれ0.10 / 対プレイヤー最小1.50 / 対敵最小0.90");
				DragFloat(u8"リング倍率", &tran.gameplay.enemySpawnRingScale, 0.01f, 0.1f, 0.9f);
				DragFloat(u8"ジッタ倍率", &tran.gameplay.enemySpawnJitterScale, 0.01f, 0.0f, 0.5f);
				DragFloat(u8"プレイヤー最小距離", &tran.gameplay.enemySpawnMinPlayerDist, 0.05f, 0.0f, 8.0f);
				DragFloat(u8"敵同士最小距離", &tran.gameplay.enemySpawnMinEnemyDist, 0.05f, 0.0f, 4.0f);

				ImGui::SeparatorText(u8"ボス共通");
				ImGui::TextDisabled(u8"共通予兆倍率は各攻撃の予兆時間に乗算、予兆幅は細突進の幅かつ全攻撃の範囲倍率");
				DragFloat(u8"ボスHPバー横幅(画面比)", &tran.gameplay.bossHpBarWidthRate, 0.005f, 0.20f, 0.90f);
				DragFloat(u8"ボスHPバー縦幅(画面比)", &tran.gameplay.bossHpBarHeightRate, 0.002f, 0.01f, 0.20f);
				ImGui::TextDisabled(u8"Breakゲージ初期値: X=0 / Y=6 / 横幅0.42 / 縦幅0.018");
				DragFloat(u8"Breakゲージ Xオフセット", &tran.gameplay.bossGuardBarOffsetX, 1.0f, -960.0f, 960.0f);
				DragFloat(u8"Breakゲージ Yオフセット", &tran.gameplay.bossGuardBarOffsetY, 1.0f, -120.0f, 320.0f);
				DragFloat(u8"Breakゲージ 横幅(画面比)", &tran.gameplay.bossGuardBarWidthRate, 0.005f, 0.10f, 0.90f);
				DragFloat(u8"Breakゲージ 縦幅(画面比)", &tran.gameplay.bossGuardBarHeightRate, 0.001f, 0.005f, 0.10f);
				DragFloat(u8"ボス面積倍率", &tran.gameplay.bossSizeAreaScale, 0.05f, 4.0f, 12.0f);
				DragInt(u8"ボス最大HP", &tran.gameplay.bossMaxHp, 1.0f, 1, 9999);
				DragFloat(u8"共通予兆倍率", &tran.gameplay.bossAttackTelegraph, 0.01f, 0.10f, 4.0f);
				DragFloat(u8"画面外ジャンプ開始", &tran.gameplay.bossAttackJumpOutTime, 0.01f, 0.0f, 4.0f);
				DragFloat(u8"ボス突進時間", &tran.gameplay.bossAttackDashDuration, 0.005f, 0.05f, 2.0f);
				DragFloat(u8"ボス攻撃CT", &tran.gameplay.bossAttackCooldown, 0.01f, 0.0f, 6.0f);
				DragFloat(u8"予兆幅(プレイヤー比)", &tran.gameplay.bossAttackLanePlayerScale, 0.05f, 0.5f, 8.0f);
				DragFloat(u8"ボス攻撃ダメージ", &tran.gameplay.bossAttackDamage, 0.1f, 0.0f, 200.0f);
				DragFloat(u8"ボス攻撃被弾ヒットストップ", &tran.gameplay.bossAttackHitStop, 0.001f, 0.0f, 0.20f);
				DragFloat(u8"ボス被弾 画面揺れ時間", &tran.gameplay.bossHitShakeDuration, 0.005f, 0.0f, 1.0f);
				DragFloat(u8"ボス被弾 画面揺れ強さ", &tran.gameplay.bossHitShakeAmplitude, 0.01f, 0.0f, 1.0f);
				DragFloat(u8"Breakゲージ 初期最大", &tran.gameplay.bossGuardInitialMax, 0.1f, 1.0f, 200.0f);
				DragFloat(u8"Breakゲージ 最終最大", &tran.gameplay.bossGuardFinalMax, 0.1f, 1.0f, 200.0f);
				DragFloat(u8"Break回復毎 上限上昇量", &tran.gameplay.bossGuardRecoverStep, 0.1f, 0.0f, 50.0f);
				DragFloat(u8"通常時 被ダメ倍率", &tran.gameplay.bossDamageScaleNormal, 0.01f, 0.0f, 5.0f);
				DragFloat(u8"Broken時 被ダメ倍率", &tran.gameplay.bossDamageScaleBroken, 0.01f, 0.0f, 10.0f);
				DragFloat(u8"Broken復帰秒", &tran.gameplay.bossBreakRecoverSec, 0.1f, 1.0f, 30.0f);

				ImGui::SeparatorText(u8"ボス: 突進");
				ImGui::TextDisabled(u8"初期値: 細予兆1.0 / 広予兆2.0 / 広幅0.50");
				DragFloat(u8"細突進 予兆秒", &tran.gameplay.bossDashNarrowTelegraph, 0.01f, 0.10f, 8.0f);
				DragFloat(u8"広突進 予兆秒", &tran.gameplay.bossDashWideTelegraph, 0.01f, 0.10f, 8.0f);
				DragFloat(u8"広突進 幅(ステージ比)", &tran.gameplay.bossDashWideWidthRate, 0.01f, 0.10f, 1.00f);

				ImGui::SeparatorText(u8"ボス: 落下・召喚");
				ImGui::TextDisabled(u8"初期値: ランダム5発 / 半径1.6 / 召喚5〜10 / 追尾5回 / 半径3.0");
				DragInt(u8"ランダム落下 回数", &tran.gameplay.bossRandomRainCount, 1.0f, 1, 16);
				DragFloat(u8"ランダム落下 予兆秒", &tran.gameplay.bossRandomRainTelegraph, 0.01f, 0.10f, 8.0f);
				DragFloat(u8"ランダム落下 半径倍率", &tran.gameplay.bossRandomRainRadiusScale, 0.05f, 0.25f, 8.0f);
				DragInt(u8"従者召喚 最小数", &tran.gameplay.bossSummonMin, 1.0f, 1, 32);
				DragInt(u8"従者召喚 最大数", &tran.gameplay.bossSummonMax, 1.0f, 1, 32);
				DragFloat(u8"従者召喚 予兆秒", &tran.gameplay.bossSummonTelegraph, 0.01f, 0.10f, 8.0f);
				DragInt(u8"追尾落下 回数", &tran.gameplay.bossTrackingDropCount, 1.0f, 1, 16);
				DragFloat(u8"追尾落下 予兆秒", &tran.gameplay.bossTrackingDropTelegraph, 0.01f, 0.10f, 8.0f);
				DragFloat(u8"追尾落下 半径倍率", &tran.gameplay.bossTrackingDropRadiusScale, 0.05f, 0.5f, 8.0f);

				ImGui::SeparatorText(u8"ボス: 必殺技");
				ImGui::TextDisabled(u8"初期値: 交差予兆1.0 / 交差幅1.0 / 踏みつけ5回 / 踏みつけ初回3.0 / 踏みつけ連続3.0 / 全体予兆7.0 / 安置2.0");
				DragFloat(u8"交差攻撃 予兆秒", &tran.gameplay.bossUltimateCrossTelegraph, 0.01f, 0.10f, 8.0f);
				DragFloat(u8"交差攻撃 幅倍率", &tran.gameplay.bossUltimateCrossLaneScale, 0.05f, 0.25f, 8.0f);
				DragInt(u8"踏みつけ 回数", &tran.gameplay.bossUltimateStompCount, 1.0f, 1, 16);
				DragFloat(u8"踏みつけ 初回予兆秒", &tran.gameplay.bossUltimateStompTelegraph, 0.01f, 0.10f, 12.0f);
				DragFloat(u8"踏みつけ 連続予兆秒", &tran.gameplay.bossUltimateStompRepeatTelegraph, 0.01f, 0.10f, 12.0f);
				DragFloat(u8"踏みつけ 半径倍率", &tran.gameplay.bossUltimateStompRadiusScale, 0.05f, 0.5f, 8.0f);
				DragFloat(u8"全体攻撃 予兆秒", &tran.gameplay.bossUltimateFieldTelegraph, 0.01f, 0.10f, 12.0f);
				DragFloat(u8"安置 半径倍率", &tran.gameplay.bossUltimateFieldSafeScale, 0.05f, 0.5f, 8.0f);

				if (tran.gameplay.bossSummonMax < tran.gameplay.bossSummonMin)
				{
					tran.gameplay.bossSummonMax = tran.gameplay.bossSummonMin;
				}

				EndTabItem();
			}
			if (BeginTabItem(u8"ゲーム調整"))
			{
				const char* difficultyText = u8"Normal";
				switch (tran.gameplayDebug.difficultyPreset)
				{
				case 0: difficultyText = u8"Easy"; break;
				case 2: difficultyText = u8"Hard"; break;
				default: difficultyText = u8"Normal"; break;
				}
				auto applyDifficultyPreset = [&](int preset)
				{
					tran.ApplyDifficultyPreset(preset);
				};
				ImGui::SeparatorText(u8"難易度プリセット");
				ImGui::Text(u8"現在: %s", difficultyText);
				if (Button(u8"Easy")) applyDifficultyPreset(0);
				SameLine();
				if (Button(u8"Normal")) applyDifficultyPreset(1);
				SameLine();
				if (Button(u8"Hard")) applyDifficultyPreset(2);

				DragInt(u8"最大Wave", &tran.gameplay.waveMax, 1.0f, 1, 32);
				DragInt(u8"Wave毎の敵追加数", &tran.gameplay.waveEnemyAddPerWave, 1.0f, 0, 16);
				DragInt(u8"強化リロール上限", &tran.roguelike.rerollMaxPerStage, 1.0f, 0, 9);
				ImGui::TextDisabled(u8"初期値: 最大Wave3 / Wave追加1");
				ImGui::SeparatorText(u8"開始カメラ演出");
				ImGui::TextDisabled(u8"初期値: 演出時間1.20秒 / フォーカス距離2.80");
				DragFloat(u8"開始演出時間", &tran.gameplay.cameraIntroDuration, 0.02f, 0.10f, 8.0f);
				DragFloat(u8"フォーカス距離", &tran.gameplay.cameraIntroFocusDistance, 0.05f, 0.50f, 12.0f);

				ImGui::SeparatorText(u8"被弾・撃破演出");
				ImGui::TextDisabled(u8"初期値: 被弾0.20 / 被弾無敵0.35 / 被弾倍率1.65 / 撃破0.28 / 撃破倍率1.60");
				DragFloat(u8"被弾フラッシュ時間", &tran.gameplay.playerDamageFlash, 0.005f, 0.0f, 1.0f);
				DragFloat(u8"被弾無敵時間", &tran.gameplay.playerDamageInvincible, 0.005f, 0.0f, 2.0f);
				DragFloat(u8"被弾フラッシュ倍率", &tran.gameplay.playerDamageFlashScale, 0.01f, 0.1f, 4.0f);
				DragFloat(u8"敵撃破フラッシュ時間", &tran.gameplay.enemyDefeatFlash, 0.005f, 0.0f, 1.0f);
				DragFloat(u8"敵撃破フラッシュ倍率", &tran.gameplay.enemyDefeatFlashScale, 0.01f, 0.1f, 4.0f);

				ImGui::SeparatorText(u8"音量");
				ImGui::TextDisabled(u8"初期値: Master1.00 / BGM0.70 / SE1.00");
				DragFloat(u8"Master音量", &tran.gameplay.volumeMaster, 0.01f, 0.0f, 2.0f);
				DragFloat(u8"BGM音量", &tran.gameplay.volumeBgm, 0.01f, 0.0f, 2.0f);
				DragFloat(u8"SE音量", &tran.gameplay.volumeSe, 0.01f, 0.0f, 2.0f);

				ImGui::SeparatorText(u8"押し合い");
				ImGui::TextDisabled(u8"初期値: 余白0.01 / プレイヤー0.55 / 敵0.45");
				DragFloat(u8"押し合い余白", &tran.gameplay.pushSlop, 0.001f, 0.0f, 0.2f);
				DragFloat(u8"プレイヤー側割合", &tran.gameplay.playerPushShare, 0.01f, 0.0f, 1.0f);
				DragFloat(u8"敵側割合", &tran.gameplay.enemyPushShare, 0.01f, 0.0f, 1.0f);

				ImGui::SeparatorText(u8"実行時デバッグ");
				ImGui::Text(u8"攻撃中: %d", tran.gameplayDebug.attackActive);
				ImGui::Text(u8"攻撃SwingID: %d", tran.gameplayDebug.attackSwingId);
				ImGui::Text(u8"このSwingヒット数: %d", tran.gameplayDebug.swingHitCount);
				ImGui::Text(u8"Wave: %d / %d", tran.gameplayDebug.currentWave, tran.gameplayDebug.maxWave);
				ImGui::Text(u8"敵数: %d / %d", tran.gameplayDebug.enemiesAlive, tran.gameplayDebug.enemiesTarget);
				ImGui::Text(u8"難易度: %s", difficultyText);
				ImGui::Text(u8"実効敵数設定: 基準%d / Wave加算%d", tran.gameplayDebug.effectiveEnemyBaseCount, tran.gameplayDebug.effectiveEnemyAddPerWave);
				ImGui::Text(u8"実効敵ダメージ: %.2f", tran.gameplayDebug.effectiveEnemyAttackDamage);
				ImGui::Text(u8"実効攻撃力: %d", tran.gameplayDebug.playerAttackDamage);
				ImGui::Text(u8"攻撃CT倍率: %.2f", tran.gameplayDebug.playerAttackCooldownScale);
				ImGui::Text(u8"回避CT倍率: %.2f", tran.gameplayDebug.playerEvadeCooldownScale);
				ImGui::Text(u8"攻撃CT進捗: %.2f", tran.gameplayDebug.cooldownRateAttack);
				ImGui::Text(u8"回避CT進捗: %.2f", tran.gameplayDebug.cooldownRateEvade);
				ImGui::Text(u8"スキル1CT進捗: %.2f", tran.gameplayDebug.cooldownRateSkill1);
				ImGui::Text(u8"スキル2CT進捗: %.2f", tran.gameplayDebug.cooldownRateSkill2);
				ImGui::Text(
					u8"装備スキル: Q=%s / E=%s",
					GetSkillNameByType(tran.roguelike.skillSlot1),
					GetSkillNameByType(tran.roguelike.skillSlot2));
				ImGui::Text(u8"回避中: %s", tran.gameplayDebug.playerEvading ? u8"はい" : u8"いいえ");
				ImGui::Text(u8"強化選択待ち: %d / リロール残り: %d", tran.gameplayDebug.upgradeSelectionPending, tran.gameplayDebug.upgradeRerollRemain);
				ImGui::Text(u8"タイマー: %.2f sec (記録 %.2f sec / 稼働 %d)", tran.gameplayDebug.runElapsedSec, tran.gameplayDebug.runRecordedSec, tran.gameplayDebug.runTimerRunning);
				ImGui::Text(u8"ボスデバッグ戦: %s", tran.gameplayDebug.bossBattleActive ? u8"ON" : u8"OFF");
				ImGui::SeparatorText(u8"HP直接操作");
				DragFloat(u8"プレイヤー現在HP", &tran.player.hp, 0.1f, 0.0f, 9999.0f);
				DragFloat(u8"プレイヤー最大HP", &tran.player.maxHp, 0.1f, 1.0f, 9999.0f);
				if (tran.player.hp < 0.0f) tran.player.hp = 0.0f;
				if (tran.player.maxHp < 1.0f) tran.player.maxHp = 1.0f;
				if (tran.player.hp > tran.player.maxHp) tran.player.hp = tran.player.maxHp;
				if (tran.gameplayDebug.bossBattleActive != 0 && tran.gameplayDebug.bossMaxHp > 0.0f)
				{
					int bossHpEdit = static_cast<int>(tran.gameplayDebug.bossHp);
					int bossMaxHpEdit = static_cast<int>(tran.gameplayDebug.bossMaxHp);
					bool bossHpEdited = false;
					bossHpEdited |= ImGui::DragInt(u8"ボス現在HP", &bossHpEdit, 1.0f, 0, 9999);
					bossHpEdited |= ImGui::DragInt(u8"ボス最大HP(実体)", &bossMaxHpEdit, 1.0f, 1, 9999);
					if (bossHpEdit < 0) bossHpEdit = 0;
					if (bossMaxHpEdit < 1) bossMaxHpEdit = 1;
					if (bossHpEdit > bossMaxHpEdit) bossHpEdit = bossMaxHpEdit;
					if (bossHpEdited)
					{
						tran.gameplayDebug.bossHpEditValue = bossHpEdit;
						tran.gameplayDebug.bossMaxHpEditValue = bossMaxHpEdit;
						tran.gameplayDebug.bossHpEditRequest = 1;
					}
				}
				else
				{
					ImGui::TextDisabled(u8"ボスHP編集はボス戦中のみ有効");
				}
				ImGui::TextDisabled(u8"ゲーム進行: 敵全滅で次Wave、最終Wave全滅で勝利");
				ImGui::TextDisabled(u8"敵タイプ差: 遠距離型は予兆後に敵弾を発射");
				ImGui::TextDisabled(u8"敵AABB色: 赤=被弾 / 橙=予兆 / 黄=攻撃可能");
				ImGui::TextDisabled(u8"敵AABB色: 水=射程内(CT中) / 緑=射程外");
				ImGui::TextDisabled(u8"射程枠(デバッグカメラ): 水=射程内 / 青=射程外");
				ImGui::SeparatorText(u8"設定保存");
				ImGui::TextDisabled("%s", tran.GetGameplayTuningPath());
				if (Button(u8"設定を保存"))
				{
					tran.SaveGameplayTuning();
				}
				SameLine();
				if (Button(u8"設定を読込"))
				{
					tran.LoadGameplayTuning();
				}

				if (Button(u8"ゲーム調整を初期値に戻す"))
				{
					tran.ResetGameplayTuningToDefault();
					tran.gameplayDebug.difficultyPreset = 1;
					tran.gameplayDebug.titleDifficultySelection = 1;
				}
				SameLine();
				if (Button(u8"強化状態をリセット"))
				{
					tran.ResetRoguelikeUpgrade();
				}

				EndTabItem();
			}
			if (BeginTabItem(u8"強化状態"))
			{
				char lastUpgradeText[64]{};
				if (tran.gameplayDebug.lastUpgradeType >= 0)
				{
					FormatUpgradeLabel(tran, tran.gameplayDebug.lastUpgradeType, lastUpgradeText, sizeof(lastUpgradeText));
				}
				else
				{
					sprintf_s(lastUpgradeText, sizeof(lastUpgradeText), "%s", u8"なし");
				}

				ImGui::Text(u8"ステージクリア回数: %d", tran.gameplayDebug.stageClearCount);
				ImGui::Text(u8"直近の強化: %s", lastUpgradeText);
				ImGui::SeparatorText(u8"強化レベル");
				ImGui::Text(u8"上限: Lv.%d", tran.GetUpgradeLevelMax());
				ImGui::Text(u8"攻撃力 Lv.%d", tran.gameplayDebug.attackPowerLevel);
				ImGui::Text(u8"攻撃頻度 Lv.%d", tran.gameplayDebug.attackSpeedLevel);
				ImGui::Text(u8"回避CT Lv.%d", tran.gameplayDebug.evadeCooldownLevel);
				ImGui::SeparatorText(u8"強化レベル編集");
				ImGui::TextDisabled(u8"ゲーム中でも直接Lvを増減できます");
				const int maxUpgradeLevel = tran.GetUpgradeLevelMax();
				int attackPowerLevelEdit = tran.roguelike.attackPowerLevel;
				int attackSpeedLevelEdit = tran.roguelike.attackSpeedLevel;
				int evadeCooldownLevelEdit = tran.roguelike.evadeCooldownLevel;
				bool upgradeLevelEdited = false;
				upgradeLevelEdited |= ImGui::DragInt(u8"攻撃力Lv##edit", &attackPowerLevelEdit, 1.0f, 0, maxUpgradeLevel);
				upgradeLevelEdited |= ImGui::DragInt(u8"攻撃頻度Lv##edit", &attackSpeedLevelEdit, 1.0f, 0, maxUpgradeLevel);
				upgradeLevelEdited |= ImGui::DragInt(u8"回避CTLv##edit", &evadeCooldownLevelEdit, 1.0f, 0, maxUpgradeLevel);
				if (Button(u8"攻撃力+1##upgrade"))
				{
					++attackPowerLevelEdit;
					upgradeLevelEdited = true;
				}
				SameLine();
				if (Button(u8"攻撃力-1##upgrade"))
				{
					--attackPowerLevelEdit;
					upgradeLevelEdited = true;
				}
				if (Button(u8"攻撃頻度+1##upgrade"))
				{
					++attackSpeedLevelEdit;
					upgradeLevelEdited = true;
				}
				SameLine();
				if (Button(u8"攻撃頻度-1##upgrade"))
				{
					--attackSpeedLevelEdit;
					upgradeLevelEdited = true;
				}
				if (Button(u8"回避CT+1##upgrade"))
				{
					++evadeCooldownLevelEdit;
					upgradeLevelEdited = true;
				}
				SameLine();
				if (Button(u8"回避CT-1##upgrade"))
				{
					--evadeCooldownLevelEdit;
					upgradeLevelEdited = true;
				}
				if (upgradeLevelEdited)
				{
					tran.roguelike.attackPowerLevel = tran.ClampUpgradeLevel(attackPowerLevelEdit);
					tran.roguelike.attackSpeedLevel = tran.ClampUpgradeLevel(attackSpeedLevelEdit);
					tran.roguelike.evadeCooldownLevel = tran.ClampUpgradeLevel(evadeCooldownLevelEdit);
					tran.gameplayDebug.attackPowerLevel = tran.roguelike.attackPowerLevel;
					tran.gameplayDebug.attackSpeedLevel = tran.roguelike.attackSpeedLevel;
					tran.gameplayDebug.evadeCooldownLevel = tran.roguelike.evadeCooldownLevel;
					tran.gameplayDebug.playerAttackDamage = tran.GetPlayerAttackDamageByLevel(tran.roguelike.attackPowerLevel);
					tran.gameplayDebug.playerAttackCooldownScale = tran.GetAttackCooldownScaleByLevel(tran.roguelike.attackSpeedLevel);
					tran.gameplayDebug.playerEvadeCooldownScale = tran.GetEvadeCooldownScaleByLevel(tran.roguelike.evadeCooldownLevel);
				}
				ImGui::SeparatorText(u8"現在の実効値");
				ImGui::Text(u8"攻撃ダメージ: %d", tran.gameplayDebug.playerAttackDamage);
				ImGui::Text(u8"攻撃CT倍率: %.2f", tran.gameplayDebug.playerAttackCooldownScale);
				ImGui::Text(u8"回避CT倍率: %.2f", tran.gameplayDebug.playerEvadeCooldownScale);
				ImGui::SeparatorText(u8"スキル強化の現在値");
				ImGui::Text(
					u8"遠距離: 範囲Lv.%d=%.2f倍 / 威力Lv.%d=%.2f倍 / CTLv.%d=-%.2f秒",
					tran.roguelike.skillShotRangeLevel,
					tran.GetSkillRangeScaleByLevel(tran.roguelike.skillShotRangeLevel),
					tran.roguelike.skillShotPowerLevel,
					tran.GetSkillDamageScaleByLevel(tran.roguelike.skillShotPowerLevel),
					tran.roguelike.skillShotCooldownLevel,
					tran.GetSkillCooldownReductionByLevel(tran.roguelike.skillShotCooldownLevel));
				ImGui::Text(
					u8"近接: 範囲Lv.%d=%.2f倍 / 威力Lv.%d=%.2f倍 / CTLv.%d=-%.2f秒",
					tran.roguelike.skillNovaRangeLevel,
					tran.GetSkillRangeScaleByLevel(tran.roguelike.skillNovaRangeLevel),
					tran.roguelike.skillNovaPowerLevel,
					tran.GetSkillDamageScaleByLevel(tran.roguelike.skillNovaPowerLevel),
					tran.roguelike.skillNovaCooldownLevel,
					tran.GetSkillCooldownReductionByLevel(tran.roguelike.skillNovaCooldownLevel));
				ImGui::Text(
					u8"衛星: 範囲Lv.%d=%.2f倍 / CTLv.%d=-%.2f秒 / 生成Lv.%d=%d個 / 威力%.2f倍",
					tran.roguelike.skillOrbitRangeLevel,
					tran.GetSkillRangeScaleByLevel(tran.roguelike.skillOrbitRangeLevel),
					tran.roguelike.skillOrbitCooldownLevel,
					tran.GetSkillCooldownReductionByLevel(tran.roguelike.skillOrbitCooldownLevel),
					tran.roguelike.skillOrbitCountLevel,
					tran.GetOrbitCountByLevel(tran.roguelike.skillOrbitCountLevel),
					tran.GetOrbitDamageScaleByCountLevel(tran.roguelike.skillOrbitCountLevel));
				ImGui::SeparatorText(u8"所持スキルのLv調整(Debug)");
				const int skillLevelMax = tran.GetSkillUpgradeLevelMax();
				bool skillUpgradeEdited = false;
				if (tran.roguelike.skillSlot1 == Transfer::RoguelikeUpgrade::SkillShot ||
					tran.roguelike.skillSlot2 == Transfer::RoguelikeUpgrade::SkillShot)
				{
					ImGui::TextDisabled(u8"遠距離スキル");
					skillUpgradeEdited |= DragInt(u8"遠距離 範囲Lv", &tran.roguelike.skillShotRangeLevel, 1.0f, 0, skillLevelMax);
					skillUpgradeEdited |= DragInt(u8"遠距離 威力Lv", &tran.roguelike.skillShotPowerLevel, 1.0f, 0, skillLevelMax);
					skillUpgradeEdited |= DragInt(u8"遠距離 CTLv", &tran.roguelike.skillShotCooldownLevel, 1.0f, 0, skillLevelMax);
				}
				if (tran.roguelike.skillSlot1 == Transfer::RoguelikeUpgrade::SkillNova ||
					tran.roguelike.skillSlot2 == Transfer::RoguelikeUpgrade::SkillNova)
				{
					ImGui::TextDisabled(u8"近接スキル");
					skillUpgradeEdited |= DragInt(u8"近接 範囲Lv", &tran.roguelike.skillNovaRangeLevel, 1.0f, 0, skillLevelMax);
					skillUpgradeEdited |= DragInt(u8"近接 威力Lv", &tran.roguelike.skillNovaPowerLevel, 1.0f, 0, skillLevelMax);
					skillUpgradeEdited |= DragInt(u8"近接 CTLv", &tran.roguelike.skillNovaCooldownLevel, 1.0f, 0, skillLevelMax);
				}
				if (tran.roguelike.skillSlot1 == Transfer::RoguelikeUpgrade::SkillOrbit ||
					tran.roguelike.skillSlot2 == Transfer::RoguelikeUpgrade::SkillOrbit)
				{
					ImGui::TextDisabled(u8"衛星スキル");
					skillUpgradeEdited |= DragInt(u8"衛星 範囲Lv", &tran.roguelike.skillOrbitRangeLevel, 1.0f, 0, skillLevelMax);
					skillUpgradeEdited |= DragInt(u8"衛星 CTLv", &tran.roguelike.skillOrbitCooldownLevel, 1.0f, 0, skillLevelMax);
					skillUpgradeEdited |= DragInt(u8"衛星 生成Lv", &tran.roguelike.skillOrbitCountLevel, 1.0f, 0, skillLevelMax);
				}
				if (tran.roguelike.skillSlot1 == Transfer::RoguelikeUpgrade::SkillNone &&
					tran.roguelike.skillSlot2 == Transfer::RoguelikeUpgrade::SkillNone)
				{
					ImGui::TextDisabled(u8"所持中のスキルがないため、ここでは調整できません");
				}
				if (skillUpgradeEdited)
				{
					tran.roguelike.skillShotRangeLevel = tran.ClampUpgradeLevel(tran.roguelike.skillShotRangeLevel);
					tran.roguelike.skillShotPowerLevel = tran.ClampUpgradeLevel(tran.roguelike.skillShotPowerLevel);
					tran.roguelike.skillShotCooldownLevel = tran.ClampUpgradeLevel(tran.roguelike.skillShotCooldownLevel);
					tran.roguelike.skillNovaRangeLevel = tran.ClampUpgradeLevel(tran.roguelike.skillNovaRangeLevel);
					tran.roguelike.skillNovaPowerLevel = tran.ClampUpgradeLevel(tran.roguelike.skillNovaPowerLevel);
					tran.roguelike.skillNovaCooldownLevel = tran.ClampUpgradeLevel(tran.roguelike.skillNovaCooldownLevel);
					tran.roguelike.skillOrbitRangeLevel = tran.ClampUpgradeLevel(tran.roguelike.skillOrbitRangeLevel);
					tran.roguelike.skillOrbitCooldownLevel = tran.ClampUpgradeLevel(tran.roguelike.skillOrbitCooldownLevel);
					tran.roguelike.skillOrbitCountLevel = tran.ClampUpgradeLevel(tran.roguelike.skillOrbitCountLevel);
				}
				ImGui::Text(u8"強化選択待ち: %s", tran.roguelike.selectionPending ? u8"あり" : u8"なし");
				ImGui::Text(u8"リロール残り: %d / %d", tran.roguelike.rerollRemain, tran.roguelike.rerollMaxPerStage);
				ImGui::SeparatorText(u8"現在の候補");
				char offerText0[64]{}, offerText1[64]{}, offerText2[64]{};
				FormatUpgradeLabel(tran, tran.roguelike.offers[0], offerText0, sizeof(offerText0));
				FormatUpgradeLabel(tran, tran.roguelike.offers[1], offerText1, sizeof(offerText1));
				FormatUpgradeLabel(tran, tran.roguelike.offers[2], offerText2, sizeof(offerText2));
				ImGui::Text(u8"[1] %s", offerText0);
				ImGui::Text(u8"[2] %s", offerText1);
				ImGui::Text(u8"[3] %s", offerText2);
				ImGui::TextDisabled(u8"勝利時に3候補から1つ選択（Rでリロール、回数上限あり）");
				if (Button(u8"強化状態をリセット##upgrade_tab"))
				{
					tran.ResetRoguelikeUpgrade();
				}
				EndTabItem();
			}
			if (BeginTabItem(u8"UI"))
			{
				ImGui::SeparatorText(u8"ポーズメニューUI");
				ImGui::TextDisabled(u8"Escで開くゲーム内メニューの表示倍率");
				DragFloat(u8"UI全体サイズ", &tran.gameplayDebug.pauseMenuUiScale, 0.01f, 0.5f, 2.5f);
				DragFloat(u8"メニュー文字サイズ", &tran.gameplayDebug.pauseMenuFontScale, 0.01f, 0.5f, 2.5f);
				DragFloat(u8"ボタンサイズ", &tran.gameplayDebug.pauseMenuButtonScale, 0.01f, 0.5f, 2.5f);
				if (Button(u8"UI設定を初期値に戻す"))
				{
					tran.gameplayDebug.pauseMenuUiScale = 1.0f;
					tran.gameplayDebug.pauseMenuFontScale = 1.0f;
					tran.gameplayDebug.pauseMenuButtonScale = 1.0f;
				}
				EndTabItem();
			}
			if (BeginTabItem(u8"モデル編集"))
			{
				ImGui::SeparatorText(u8"腕");
				if(TreeNode(u8"腕パーツ"))
				{
					PushID(0);
					if(TreeNode(u8"右腕"))
					{
						ImGui::DragFloat3(u8"右腕1 角度", reinterpret_cast<float*>(&tran.modelediter.armRight1.subAngle), 0.1f);
						ImGui::DragFloat3(u8"右腕1 位置", reinterpret_cast<float*>(&tran.modelediter.armRight1.pos), 0.1f);
						ImGui::DragFloat3(u8"右腕1 サイズ", reinterpret_cast<float*>(&tran.modelediter.armRight1.size), 0.1f);
						ImGui::DragFloat3(u8"右腕1 回転", reinterpret_cast<float*>(&tran.modelediter.armRight1.rotate), 0.1f);
						ImGui::SeparatorText(u8"右腕2");
						ImGui::DragFloat3(u8"右腕2 角度", reinterpret_cast<float*>(&tran.modelediter.armRight2.subAngle), 0.1f);
						ImGui::DragFloat3(u8"右腕2 位置", reinterpret_cast<float*>(&tran.modelediter.armRight2.pos), 0.1f);
						ImGui::DragFloat3(u8"右腕2 サイズ", reinterpret_cast<float*>(&tran.modelediter.armRight2.size), 0.1f);
						ImGui::DragFloat3(u8"右腕2 回転", reinterpret_cast<float*>(&tran.modelediter.armRight2.rotate), 0.1f);
						TreePop();
					}
					if(TreeNode(u8"左腕"))
					{
						ImGui::SeparatorText(u8"左腕1");
						ImGui::DragFloat3(u8"左腕1 角度", reinterpret_cast<float*>(&tran.modelediter.armLeft1.subAngle), 0.1f);
						ImGui::DragFloat3(u8"左腕1 位置", reinterpret_cast<float*>(&tran.modelediter.armLeft1.pos), 0.1f);
						ImGui::DragFloat3(u8"左腕1 サイズ", reinterpret_cast<float*>(&tran.modelediter.armLeft1.size), 0.1f);
						ImGui::DragFloat3(u8"左腕1 回転", reinterpret_cast<float*>(&tran.modelediter.armLeft1.rotate), 0.1f);
						ImGui::SeparatorText(u8"左腕2");
						ImGui::DragFloat3(u8"左腕2 角度", reinterpret_cast<float*>(&tran.modelediter.armLeft2.subAngle), 0.1f);
						ImGui::DragFloat3(u8"左腕2 位置", reinterpret_cast<float*>(&tran.modelediter.armLeft2.pos), 0.1f);
						ImGui::DragFloat3(u8"左腕2 サイズ", reinterpret_cast<float*>(&tran.modelediter.armLeft2.size), 0.1f);
						ImGui::DragFloat3(u8"左腕2 回転", reinterpret_cast<float*>(&tran.modelediter.armLeft2.rotate), 0.1f);
						TreePop();
					}
					TreePop();
					PopID();
				}
				ImGui::SeparatorText(u8"脚");
				if(TreeNode(u8"脚パーツ"))
				{
					PushID(1);
					if(TreeNode(u8"右脚"))
					{ 
						ImGui::DragFloat3(u8"右脚1 角度", reinterpret_cast<float*>(&tran.modelediter.legRight1.subAngle), 0.1f);
						ImGui::DragFloat3(u8"右脚1 位置", reinterpret_cast<float*>(&tran.modelediter.legRight1.pos), 0.1f);
						ImGui::DragFloat3(u8"右脚1 サイズ", reinterpret_cast<float*>(&tran.modelediter.legRight1.size), 0.1f);
						ImGui::DragFloat3(u8"右脚1 回転", reinterpret_cast<float*>(&tran.modelediter.legRight1.rotate), 0.1f);
						ImGui::SeparatorText(u8"右脚2");
						ImGui::DragFloat3(u8"右脚2 角度", reinterpret_cast<float*>(&tran.modelediter.legRight2.subAngle), 0.1f);
						ImGui::DragFloat3(u8"右脚2 位置", reinterpret_cast<float*>(&tran.modelediter.legRight2.pos), 0.1f);
						ImGui::DragFloat3(u8"右脚2 サイズ", reinterpret_cast<float*>(&tran.modelediter.legRight2.size), 0.1f);
						ImGui::DragFloat3(u8"右脚2 回転", reinterpret_cast<float*>(&tran.modelediter.legRight2.rotate), 0.1f);
						TreePop();
					}
					if(TreeNode(u8"左脚"))
					{ 
						ImGui::SeparatorText(u8"左脚1");
						ImGui::DragFloat3(u8"左脚1 角度", reinterpret_cast<float*>(&tran.modelediter.legLeft1.subAngle), 0.1f);
						ImGui::DragFloat3(u8"左脚1 位置", reinterpret_cast<float*>(&tran.modelediter.legLeft1.pos), 0.1f);
						ImGui::DragFloat3(u8"左脚1 サイズ", reinterpret_cast<float*>(&tran.modelediter.legLeft1.size), 0.1f);
						ImGui::DragFloat3(u8"左脚1 回転", reinterpret_cast<float*>(&tran.modelediter.legLeft1.rotate), 0.1f);
						ImGui::SeparatorText(u8"左脚2");
						ImGui::DragFloat3(u8"左脚2 角度", reinterpret_cast<float*>(&tran.modelediter.legLeft2.subAngle), 0.1f);
						ImGui::DragFloat3(u8"左脚2 位置", reinterpret_cast<float*>(&tran.modelediter.legLeft2.pos), 0.1f);
						ImGui::DragFloat3(u8"左脚2 サイズ", reinterpret_cast<float*>(&tran.modelediter.legLeft2.size), 0.1f);
						ImGui::DragFloat3(u8"左脚2 回転", reinterpret_cast<float*>(&tran.modelediter.legLeft2.rotate), 0.1f);
						TreePop();
					}
					TreePop();
					PopID();
				}

				ImGui::SeparatorText(u8"胴体");
				ImGui::DragFloat3(u8"胴体 位置", reinterpret_cast<float*>(&tran.modelediter.body.pos), 0.1f);
				ImGui::DragFloat3(u8"胴体 サイズ", reinterpret_cast<float*>(&tran.modelediter.body.size), 0.1f);
				ImGui::DragFloat3(u8"胴体 回転", reinterpret_cast<float*>(&tran.modelediter.body.angle), 0.1f);
				ImGui::DragFloat3(u8"右腕ジョイント", reinterpret_cast<float*>(&tran.modelediter.body.jointRightArmPos),0.1f);
				ImGui::DragFloat3(u8"左腕ジョイント", reinterpret_cast<float*>(&tran.modelediter.body.jointLeftArmPos),0.1f);
				ImGui::DragFloat3(u8"右脚ジョイント", reinterpret_cast<float*>(&tran.modelediter.body.jointRightLegPos),0.1f);
				ImGui::DragFloat3(u8"左脚ジョイント", reinterpret_cast<float*>(&tran.modelediter.body.jointLeftLegPos),0.1f);

				EndTabItem();
			}
			if (BeginTabItem(u8"カメラ"))
            {

                static float min = 20.0f;
                static float max = 80.0f;

                DragFloatRange2(u8"仮レンジ", &min, &max, 0.1f, 0.0f, 100.0f);

                const char* cameraModes[] = { u8"ゲーム", u8"デバッグ" };
                int mode = tran.cameraMode;
                if (Combo(u8"カメラモード", &mode, cameraModes, IM_ARRAYSIZE(cameraModes)))
                {
                    tran.cameraMode = mode;
                }

                const char* activeLabel = (tran.cameraMode == 1) ? u8"デバッグ" : u8"ゲーム";
                Text(u8"現在: %s", activeLabel);

                SeparatorText(u8"ゲームカメラ");
                float gameEye[3] = { tran.cameraGame.eye.x, tran.cameraGame.eye.y, tran.cameraGame.eye.z };
                float gameLook[3] = { tran.cameraGame.look.x, tran.cameraGame.look.y, tran.cameraGame.look.z };
                if (DragFloat3(u8"ゲーム Eye", gameEye, 0.05f))
                {
                    tran.cameraGame.eye = { gameEye[0], gameEye[1], gameEye[2] };
                }
                if (DragFloat3(u8"ゲーム Look", gameLook, 0.05f))
                {
                    tran.cameraGame.look = { gameLook[0], gameLook[1], gameLook[2] };
                }

                SeparatorText(u8"デバッグカメラ");
                float debugEye[3] = { tran.cameraDebug.eye.x, tran.cameraDebug.eye.y, tran.cameraDebug.eye.z };
                float debugLook[3] = { tran.cameraDebug.look.x, tran.cameraDebug.look.y, tran.cameraDebug.look.z };
                if (DragFloat3(u8"デバッグ Eye", debugEye, 0.05f))
                {
                    tran.cameraDebug.eye = { debugEye[0], debugEye[1], debugEye[2] };
                }
                if (DragFloat3(u8"デバッグ Look", debugLook, 0.05f))
                {
                    tran.cameraDebug.look = { debugLook[0], debugLook[1], debugLook[2] };
                }

                EndTabItem();
            }
			if (BeginTabItem(u8"グラフ"))
			{
				static float easing = 1;

				DragFloat(u8"Exp減衰", &easing,0.1f);
				Separator();

				float target = 1000;
				float current = 10;
				float frame[100];
				for (int i = 0; i < 100; i++)
				{
					current = current + (target - current) * expf(-easing);
					frame[i] = current;
				}

				PlotLines(u8"expf", frame, 100, 0, 0, 3.4028235E38F, 3.4028235E38F, { 100,100 }, 4);
				Separator();
				current = 0;
				float PlanA[100];
				for (int i = 0; i < 100; i++)
				{
					current += 0.01f;
					PlanA[i] = current * current * current * current;
				}

				PlotLines(u8"プランA",PlanA,100, 0, 0, 3.4028235E38F, 3.4028235E38F, { 100,100 }, 4);
				Separator();
				float c4 = (2 * PI) / 3;

				current = 0;
				float  PlanB[100];
				for (int i = 0; i < 100; i++)
				{
					current += 0.01f;
					PlanB[i] = current == 0.0f ? 0.0f : current == 1.0f ? 1.0f : powf(2, -10 * current) * sinf((current * 10 - 0.75f) * c4) + 1.0f;
				}
				PlotLines(u8"プランB", PlanB, 100, 0, 0, 3.4028235E38F, 3.4028235E38F, { 100,100 }, 4);

				Separator();

				current = 0;

				float n1 = 7.5625;
				float d1 = 2.75;
				float PlanC[100];
				for(int i = 0; i < 100;i++)
				{
					current += 0.01f;
					PlanC[i] = 1.0f - EaseOutBounce(current);
				}

				PlotLines(u8"プランC", PlanC, 100, 0, 0, 3.4028235E38F, 3.4028235E38F, { 100,100 }, 4);

				EndTabItem();
			}
			if (BeginTabItem(u8"シーン切替"))
			{
				static int sceneNum = 0;
				if (ArrowButton("上", ImGuiDir_Up))
					sceneNum++;
				SameLine();
				if (ArrowButton("下", ImGuiDir_Down))
					sceneNum--;
				if (sceneNum < 0)sceneNum = static_cast<int>(SceneManager::SceneType::SCENE_MAX) - 1;
				sceneNum = sceneNum % static_cast<int>(SceneManager::SceneType::SCENE_MAX);
				SceneManager::SceneType changeScene;
				switch (sceneNum)
				{
				case 0:
					sceneTxt = u8"タイトル";
					changeScene = SceneManager::SceneType::SCENE_TITLE;
					break;
				case 1:
					sceneTxt = u8"ゲーム";
					changeScene = SceneManager::SceneType::SCENE_GAME;
					break;
				case 2:
					sceneTxt = u8"リザルト";
					changeScene = SceneManager::SceneType::SCENE_RESULT;
					break;
				case 3:
					sceneTxt = u8"城エディタ";
					changeScene = SceneManager::SceneType::SCENE_ENGINE_EDITOR;
					break;
				default:
					sceneTxt = u8"不明";
					break;
				}
				sceneTxt = u8"変更先: " + sceneTxt;
				SameLine();
				Text(sceneTxt.c_str());
				if (Button(u8"適用"))
					SceneManager::ChangeScene(changeScene);

				EndTabItem();
			}
			if (BeginTabItem(u8"マウス"))
			{
				std::string mouseInfoText;

				ImGuiIO& io = ImGui::GetIO();
				tran.mousePos.x = ImGui::GetCursorScreenPos().x;
				tran.mousePos.y = ImGui::GetCursorScreenPos().y;
				mouseInfoText = std::to_string(tran.mousePos.x) + " : " + std::to_string(tran.mousePos.y);
				ImGui::Text(mouseInfoText.c_str());

				if (ImGui::IsMousePosValid())
					ImGui::Text(u8"マウス座標: (%g, %g)", io.MousePos.x, io.MousePos.y);
				EndTabItem();
			}
			EndTabBar();
		}
		Separator();
		Text(u8"FPS: %.1f", GetIO().Framerate);
		End();
	}	// -----------------------------

	DrawEngineEditorWindows();

	// Debug Tools : Tables + DrawList
	//   - Tables : 一覧/監視用
	//   - DrawList : 画面上への簡易オーバーレイ
	// -----------------------------
	static bool show_table_window = true;
	static bool show_overlay = true;
	static bool show_imgui_demo = false;

	// TABキーのメインウィンドウだけだと隠れやすいので、ここで簡易トグルも用意
	if (ImGui::IsKeyPressed(ImGuiKey_F1, false)) show_table_window = !show_table_window;
	if (ImGui::IsKeyPressed(ImGuiKey_F2, false)) show_overlay = !show_overlay;
	if (ImGui::IsKeyPressed(ImGuiKey_F3, false)) show_imgui_demo = !show_imgui_demo;

	if (show_table_window)
	{
		ImGui::Begin(u8"インスペクタ（表）", &show_table_window);

		ImGui::Text(u8"F1:表  F2:オーバーレイ");
		ImGui::Separator();

		// 1) Key-Value 監視テーブル（縦スクロール）
		ImGuiTableFlags flags =
			ImGuiTableFlags_Borders |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Resizable |
			ImGuiTableFlags_SizingStretchProp |
			ImGuiTableFlags_ScrollY;

		const float table_h = 220.0f;
		if (ImGui::BeginTable("##kv", 2, flags, ImVec2(0.0f, table_h)))
		{
			ImGui::TableSetupColumn(u8"項目", ImGuiTableColumnFlags_WidthFixed, 180.0f);
			ImGui::TableSetupColumn(u8"値", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();

			auto row_f3 = [](const char* name, const DirectX::XMFLOAT3& v)
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(name);
					ImGui::TableSetColumnIndex(1); ImGui::Text("(%.2f, %.2f, %.2f)", v.x, v.y, v.z);
				};
			auto row_f4 = [](const char* name, const DirectX::XMFLOAT4& v)
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(name);
					ImGui::TableSetColumnIndex(1); ImGui::Text("(%.2f, %.2f, %.2f, %.2f)", v.x, v.y, v.z, v.w);
				};
			auto row_f1 = [](const char* name, float v)
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(name);
					ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f", v);
				};
			auto row_i1 = [](const char* name, int v)
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(name);
					ImGui::TableSetColumnIndex(1); ImGui::Text("%d", v);
				};
			auto row_s1 = [](const char* name, const char* v)
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(name);
					ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(v ? v : "-");
				};
			// Transfer の中身を例として監視
			row_f3(u8"プレイヤー位置", tran.player.pos);
			row_f3(u8"プレイヤー速度", tran.player.velocity);
			row_f1(u8"プレイヤーHP", tran.player.hp);
			row_f1(u8"プレイヤー最大HP", tran.player.maxHp);

			row_i1(u8"敵存在", tran.enemy.exists);
			row_f3(u8"敵位置", tran.enemy.pos);
			row_f1(u8"敵HP", tran.enemy.hp);
			row_f1(u8"敵最大HP", tran.enemy.maxHp);
			const char* enemyStateText = u8"なし";
			switch (tran.enemy.state)
			{
			case 0: enemyStateText = u8"徘徊"; break;
			case 1: enemyStateText = u8"追跡"; break;
			default: enemyStateText = u8"なし"; break;
			}
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(u8"敵状態");
			ImGui::TableSetColumnIndex(1); ImGui::Text("%d (%s)", tran.enemy.state, enemyStateText);
			const char* enemyTypeText = u8"なし";
			switch (tran.enemy.type)
			{
			case 0: enemyTypeText = u8"速度型"; break;
			case 1: enemyTypeText = u8"耐久型"; break;
			case 2: enemyTypeText = u8"遠距離型"; break;
			default: enemyTypeText = u8"なし"; break;
			}
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(u8"敵タイプ");
			ImGui::TableSetColumnIndex(1); ImGui::Text("%d (%s)", tran.enemy.type, enemyTypeText);


			row_f3(u8"サイコロ位置", tran.dice.pos);
			row_f3(u8"サイコロ速度", tran.dice.velocity);
			row_f4(u8"サイコロ回転", tran.dice.rot);
			row_f1(u8"停止しきい値速度", tran.dice.underVel);

			const char* cameraModeText = (tran.cameraMode == 1) ? u8"デバッグ" : u8"ゲーム";
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(u8"カメラモード");
			ImGui::TableSetColumnIndex(1); ImGui::Text("%d (%s)", tran.cameraMode, cameraModeText);
			row_i1(u8"現在Wave", tran.gameplayDebug.currentWave);
			row_i1(u8"最大Wave", tran.gameplayDebug.maxWave);
			row_i1(u8"生存敵数", tran.gameplayDebug.enemiesAlive);
			row_i1(u8"目標敵数", tran.gameplayDebug.enemiesTarget);
			const char* difficultyPresetText = u8"ノーマル";
			switch (tran.gameplayDebug.difficultyPreset)
			{
			case 0: difficultyPresetText = u8"イージー"; break;
			case 2: difficultyPresetText = u8"ハード"; break;
			default: difficultyPresetText = u8"ノーマル"; break;
			}
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(u8"難易度プリセット");
			ImGui::TableSetColumnIndex(1); ImGui::Text("%d (%s)", tran.gameplayDebug.difficultyPreset, difficultyPresetText);
			row_i1(u8"実効基準敵数", tran.gameplayDebug.effectiveEnemyBaseCount);
			row_i1(u8"実効Wave追加敵数", tran.gameplayDebug.effectiveEnemyAddPerWave);
			row_f1(u8"実効敵攻撃ダメージ", tran.gameplayDebug.effectiveEnemyAttackDamage);
			row_i1(u8"実効プレイヤー攻撃力", tran.gameplayDebug.playerAttackDamage);
			row_f1(u8"実効プレイヤー攻撃CT倍率", tran.gameplayDebug.playerAttackCooldownScale);
			row_f1(u8"実効プレイヤー回避CT倍率", tran.gameplayDebug.playerEvadeCooldownScale);
			row_i1(u8"プレイヤー回避中", tran.gameplayDebug.playerEvading);
			row_i1(u8"ステージクリア回数", tran.gameplayDebug.stageClearCount);
			row_i1(u8"強化:攻撃力Lv", tran.gameplayDebug.attackPowerLevel);
			row_i1(u8"強化:攻撃頻度Lv", tran.gameplayDebug.attackSpeedLevel);
			row_i1(u8"強化:回避CTLv", tran.gameplayDebug.evadeCooldownLevel);
			row_i1(u8"強化選択待ち", tran.gameplayDebug.upgradeSelectionPending);
			row_i1(u8"強化リロール残り", tran.gameplayDebug.upgradeRerollRemain);
			row_f1(u8"タイマー経過秒", tran.gameplayDebug.runElapsedSec);
			row_f1(u8"タイマー記録秒", tran.gameplayDebug.runRecordedSec);
			row_i1(u8"タイマー稼働中", tran.gameplayDebug.runTimerRunning);
			row_i1(u8"ボスデバッグ戦中", tran.gameplayDebug.bossBattleActive);
			row_f1(u8"ボスHP", tran.gameplayDebug.bossHp);
			row_f1(u8"ボス最大HP", tran.gameplayDebug.bossMaxHp);
			row_s1(u8"カーソル対象", tran.gameplayDebug.cursorHoverTarget);
			char debugOffer0[64]{}, debugOffer1[64]{}, debugOffer2[64]{};
			FormatUpgradeLabel(tran, tran.gameplayDebug.upgradeOffer0, debugOffer0, sizeof(debugOffer0));
			FormatUpgradeLabel(tran, tran.gameplayDebug.upgradeOffer1, debugOffer1, sizeof(debugOffer1));
			FormatUpgradeLabel(tran, tran.gameplayDebug.upgradeOffer2, debugOffer2, sizeof(debugOffer2));
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(u8"強化候補1");
			ImGui::TableSetColumnIndex(1); ImGui::Text("%d (%s)", tran.gameplayDebug.upgradeOffer0, debugOffer0);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(u8"強化候補2");
			ImGui::TableSetColumnIndex(1); ImGui::Text("%d (%s)", tran.gameplayDebug.upgradeOffer1, debugOffer1);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(u8"強化候補3");
			ImGui::TableSetColumnIndex(1); ImGui::Text("%d (%s)", tran.gameplayDebug.upgradeOffer2, debugOffer2);

			ImGui::EndTable();
		}

		ImGui::Spacing();

		if (ImGui::CollapsingHeader(u8"ステータス強化（表）", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::TextDisabled(u8"左2列固定。幅が足りない場合は横スクロール");
			DrawStatusUpgradeValueTable(tran);
		}
		if (ImGui::CollapsingHeader(u8"スキル強化（表）", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::TextDisabled(u8"左2列固定。列は Lv1 から Lv10 の強化段階");
			DrawSkillUpgradeValueTable(tran);
		}

		ImGui::End();
	}

	if (SceneManager::GetCurrent() == SceneManager::SceneType::SCENE_GAME)
	{
		ImGuiViewport* vp = ImGui::GetMainViewport();
		ImDrawList* dl = ImGui::GetForegroundDrawList(vp);
		if (tran.gameplayDebug.bossBattleActive != 0 && tran.gameplayDebug.bossMaxHp > 0.0f)
		{
			float widthRate = tran.gameplay.bossHpBarWidthRate;
			if (widthRate < 0.20f) widthRate = 0.20f;
			if (widthRate > 0.90f) widthRate = 0.90f;
			float heightRate = tran.gameplay.bossHpBarHeightRate;
			if (heightRate < 0.01f) heightRate = 0.01f;
			if (heightRate > 0.20f) heightRate = 0.20f;
			float guardWidthRate = tran.gameplay.bossGuardBarWidthRate;
			if (guardWidthRate < 0.10f) guardWidthRate = 0.10f;
			if (guardWidthRate > 0.90f) guardWidthRate = 0.90f;
			float guardHeightRate = tran.gameplay.bossGuardBarHeightRate;
			if (guardHeightRate < 0.005f) guardHeightRate = 0.005f;
			if (guardHeightRate > 0.10f) guardHeightRate = 0.10f;

			float hpRate = tran.gameplayDebug.bossHp / tran.gameplayDebug.bossMaxHp;
			if (hpRate < 0.0f) hpRate = 0.0f;
			if (hpRate > 1.0f) hpRate = 1.0f;
			float guardRate = 0.0f;
			if (tran.gameplayDebug.bossGuardMax > 0.0f)
			{
				guardRate = tran.gameplayDebug.bossGuard / tran.gameplayDebug.bossGuardMax;
			}
			if (guardRate < 0.0f) guardRate = 0.0f;
			if (guardRate > 1.0f) guardRate = 1.0f;

			const float barW = vp->Size.x * widthRate;
			const float barH = vp->Size.y * heightRate;
			const float x = vp->Pos.x + (vp->Size.x - barW) * 0.5f;
			const float y = vp->Pos.y + 12.0f;
			const float padding = 4.0f;
			const float radius = 6.0f;

			const ImVec2 frameMin(x, y);
			const ImVec2 frameMax(x + barW, y + barH);
			dl->AddRectFilled(frameMin, frameMax, IM_COL32(20, 20, 20, 220), radius);
			dl->AddRect(frameMin, frameMax, IM_COL32(230, 230, 230, 210), radius, 0, 2.0f);

			const float innerW = (barW - padding * 2.0f) * hpRate;
			if (innerW > 0.0f)
			{
				dl->AddRectFilled(
					ImVec2(x + padding, y + padding),
					ImVec2(x + padding + innerW, y + barH - padding),
					IM_COL32(180, 30, 30, 220),
					radius * 0.6f);
			}
			dl->AddText(ImVec2(x + 8.0f, y - 18.0f), IM_COL32(255, 255, 255, 230), "BOSS");

			const float guardBarW = vp->Size.x * guardWidthRate;
			const float guardBarH = (vp->Size.y * guardHeightRate < 6.0f) ? 6.0f : (vp->Size.y * guardHeightRate);
			const float guardX = vp->Pos.x + (vp->Size.x - guardBarW) * 0.5f + tran.gameplay.bossGuardBarOffsetX;
			const float guardY = vp->Pos.y + 12.0f + barH + tran.gameplay.bossGuardBarOffsetY;
			dl->AddRectFilled(ImVec2(guardX, guardY), ImVec2(guardX + guardBarW, guardY + guardBarH), IM_COL32(18, 18, 18, 210), radius * 0.45f);
			dl->AddRect(ImVec2(guardX, guardY), ImVec2(guardX + guardBarW, guardY + guardBarH), IM_COL32(210, 210, 210, 180), radius * 0.45f, 0, 1.5f);

			const float guardInnerW = (guardBarW - padding * 2.0f) * guardRate;
			if (guardInnerW > 0.0f)
			{
				const ImU32 guardColor = (tran.gameplayDebug.bossBroken != 0)
					? IM_COL32(130, 130, 130, 220)
					: IM_COL32(50, 175, 255, 220);
				dl->AddRectFilled(
					ImVec2(guardX + padding, guardY + padding * 0.35f),
					ImVec2(guardX + padding + guardInnerW, guardY + guardBarH - padding * 0.35f),
					guardColor,
					radius * 0.3f);
			}
			dl->AddText(ImVec2(guardX + 8.0f, guardY - 16.0f), IM_COL32(180, 225, 255, 220), "GUARD");
			if (tran.gameplayDebug.bossBroken != 0)
			{
				dl->AddText(ImVec2(guardX + guardBarW - 78.0f, guardY - 16.0f), IM_COL32(255, 220, 120, 230), "BROKEN");
			}
		}
	}
	if (SceneManager::GetCurrent() == SceneManager::SceneType::SCENE_GAME &&
		tran.gameplayDebug.pauseMenuOpen != 0)
	{
		const float uiScale = (tran.gameplayDebug.pauseMenuUiScale < 0.5f) ? 0.5f
			: (tran.gameplayDebug.pauseMenuUiScale > 2.5f ? 2.5f : tran.gameplayDebug.pauseMenuUiScale);
		const float fontScale = (tran.gameplayDebug.pauseMenuFontScale < 0.5f) ? 0.5f
			: (tran.gameplayDebug.pauseMenuFontScale > 2.5f ? 2.5f : tran.gameplayDebug.pauseMenuFontScale);
		const float buttonScale = (tran.gameplayDebug.pauseMenuButtonScale < 0.5f) ? 0.5f
			: (tran.gameplayDebug.pauseMenuButtonScale > 2.5f ? 2.5f : tran.gameplayDebug.pauseMenuButtonScale);
		ImGuiViewport* vp = ImGui::GetMainViewport();
		const bool pauseOptionOpen = (tran.gameplayDebug.pauseOptionOpen != 0);
		const int selectedTab = (tran.gameplayDebug.pauseTabIndex < 0) ? 0
			: (tran.gameplayDebug.pauseTabIndex > 2 ? 2 : tran.gameplayDebug.pauseTabIndex);
		const ImVec2 windowSize = pauseOptionOpen
			? ImVec2(700.0f * uiScale, 520.0f * uiScale)
			: ImVec2(640.0f * uiScale, 500.0f * uiScale);
		const ImVec2 windowPos(vp->Pos.x + (vp->Size.x - windowSize.x) * 0.5f,
							   vp->Pos.y + (vp->Size.y - windowSize.y) * 0.5f);
		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18.0f * uiScale, 16.0f * uiScale));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f * uiScale, 12.0f * uiScale));
		ImGui::Begin("##pause_menu_overlay", nullptr,
					 ImGuiWindowFlags_NoTitleBar |
					 ImGuiWindowFlags_NoCollapse |
					 ImGuiWindowFlags_NoResize |
					 ImGuiWindowFlags_NoMove |
					 ImGuiWindowFlags_NoDocking);
		ImGui::SetWindowFontScale(fontScale);
		ImGui::TextUnformatted(u8"ポーズ");
		ImGui::Separator();
		if (ImGui::BeginTabBar("##pause_tabs", ImGuiTabBarFlags_FittingPolicyShrink))
		{
			if (ImGui::BeginTabItem(u8"ゲーム", nullptr, (selectedTab == 0) ? ImGuiTabItemFlags_SetSelected : 0))
			{
				tran.gameplayDebug.pauseTabIndex = 0;
				const char* difficultyText = GetDifficultyName(tran.NormalizeDifficultyPreset(tran.gameplayDebug.difficultyPreset));
				const int enemiesAlive = (tran.gameplayDebug.enemiesAlive < 0) ? 0 : tran.gameplayDebug.enemiesAlive;
				const int enemiesTarget = (tran.gameplayDebug.enemiesTarget < 0) ? 0 : tran.gameplayDebug.enemiesTarget;
				int defeated = enemiesTarget - enemiesAlive;
				if (defeated < 0) defeated = 0;
				if (defeated > enemiesTarget) defeated = enemiesTarget;
				const float defeatRate = (enemiesTarget > 0)
					? (static_cast<float>(defeated) / static_cast<float>(enemiesTarget)) * 100.0f
					: 0.0f;

				ImGui::Text(u8"現在Wave: %d / %d", tran.gameplayDebug.currentWave, tran.gameplayDebug.maxWave);
				ImGui::Text(u8"難易度: %s", difficultyText);
				ImGui::Text(u8"敵撃破率: %.0f%%", defeatRate);
				if (tran.gameplayDebug.bossBattleActive != 0)
				{
					ImGui::Spacing();
					ImGui::TextUnformatted(u8"現在はボス戦状態です。");
				}
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::TextUnformatted(u8"タブ切替: 1 / 2 / 3");
				ImGui::TextUnformatted(u8"コントローラー: LB / RB");
				ImGui::TextUnformatted(u8"閉じる: Esc");
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(u8"強化状態", nullptr, (selectedTab == 1) ? ImGuiTabItemFlags_SetSelected : 0))
			{
				tran.gameplayDebug.pauseTabIndex = 1;
				ImGui::Text(u8"攻撃Lv: %d", tran.roguelike.attackPowerLevel);
				ImGui::Text(u8"攻撃頻度Lv: %d", tran.roguelike.attackSpeedLevel);
				ImGui::Text(u8"回避Lv: %d", tran.roguelike.evadeCooldownLevel);
				if (tran.roguelike.lastUpgradeType >= 0)
				{
					char lastUpgradeText[128]{};
					FormatUpgradeLabel(tran, tran.roguelike.lastUpgradeType, lastUpgradeText, sizeof(lastUpgradeText));
					ImGui::Text(u8"最終取得: %s", lastUpgradeText);
				}

				const auto drawSkillStatus = [&](const char* slotLabel, int skillType)
				{
					char detail[128]{};
					switch (skillType)
					{
					case Transfer::RoguelikeUpgrade::SkillShot:
						sprintf_s(detail, sizeof(detail), u8"範囲Lv%d / 威力Lv%d / CTLv%d",
								  tran.roguelike.skillShotRangeLevel,
								  tran.roguelike.skillShotPowerLevel,
								  tran.roguelike.skillShotCooldownLevel);
						break;
					case Transfer::RoguelikeUpgrade::SkillNova:
						sprintf_s(detail, sizeof(detail), u8"範囲Lv%d / 威力Lv%d / CTLv%d",
								  tran.roguelike.skillNovaRangeLevel,
								  tran.roguelike.skillNovaPowerLevel,
								  tran.roguelike.skillNovaCooldownLevel);
						break;
					case Transfer::RoguelikeUpgrade::SkillOrbit:
						sprintf_s(detail, sizeof(detail), u8"範囲Lv%d / CTLv%d / 個数Lv%d",
								  tran.roguelike.skillOrbitRangeLevel,
								  tran.roguelike.skillOrbitCooldownLevel,
								  tran.roguelike.skillOrbitCountLevel);
						break;
					default:
						sprintf_s(detail, sizeof(detail), "%s", u8"未取得");
						break;
					}

					ImGui::SeparatorText(slotLabel);
					ImGui::Text(u8"スキル: %s", GetSkillNameByType(skillType));
					ImGui::TextUnformatted(detail);
				};

				drawSkillStatus("Q", tran.roguelike.skillSlot1);
				drawSkillStatus("E", tran.roguelike.skillSlot2);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(u8"設定", nullptr, (selectedTab == 2) ? ImGuiTabItemFlags_SetSelected : 0))
			{
				tran.gameplayDebug.pauseTabIndex = 2;
				if (pauseOptionOpen)
				{
					const int selected = tran.gameplayDebug.pauseOptionSelection;
					const bool isFullscreen = IsAppFullscreen();
					const float closeButtonWidth = 88.0f * uiScale;
					ImGui::TextUnformatted(u8"Option");
					ImGui::SameLine();
					ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - closeButtonWidth);
					if (ImGui::Button("Close##pause_option_close", ImVec2(closeButtonWidth, 0.0f)))
					{
						tran.gameplayDebug.pauseOptionRequestClose = 1;
					}
					ImGui::Separator();

					const char* selectedLabel = u8"Master";
					switch (selected)
					{
					case 1: selectedLabel = u8"BGM"; break;
					case 2: selectedLabel = u8"SE"; break;
					case 3: selectedLabel = u8"表示"; break;
					case 4: selectedLabel = u8"戻る"; break;
					default: break;
					}
					ImGui::Text(u8"選択中: %s", selectedLabel);

					float master = tran.gameplay.volumeMaster;
					if (ImGui::SliderFloat(u8"Master", &master, 0.0f, 2.0f, "%.2f"))
					{
						tran.gameplay.volumeMaster = master;
					}
					float bgm = tran.gameplay.volumeBgm;
					if (ImGui::SliderFloat(u8"BGM", &bgm, 0.0f, 2.0f, "%.2f"))
					{
						tran.gameplay.volumeBgm = bgm;
					}
					float se = tran.gameplay.volumeSe;
					if (ImGui::SliderFloat(u8"SE", &se, 0.0f, 2.0f, "%.2f"))
					{
						tran.gameplay.volumeSe = se;
					}

					bool fullscreenChecked = isFullscreen;
					bool windowChecked = !isFullscreen;
					if (ImGui::Checkbox(u8"Fullscreen", &fullscreenChecked))
					{
						SetAppFullscreen(fullscreenChecked);
					}
					ImGui::SameLine();
					if (ImGui::Checkbox(u8"Window", &windowChecked))
					{
						SetAppFullscreen(!windowChecked);
					}
					ImGui::Separator();
					ImGui::TextUnformatted(u8"移動: W/S or ↑/↓");
					ImGui::TextUnformatted(u8"変更: A/D or ←/→");
					ImGui::TextUnformatted(u8"決定: Enter / F / Space");
					ImGui::TextUnformatted(u8"戻る: Esc");
				}
				else
				{
					const int selected = tran.gameplayDebug.pauseMenuSelection;
					ImGui::TextUnformatted(u8"設定項目");
					ImGui::Separator();
					ImGui::TextUnformatted(u8"選択: A/D W/S ←→↑↓");
					ImGui::TextUnformatted(u8"決定: Enter / F / Space");
					ImGui::TextUnformatted(u8"タブ切替: 1 / 2 / 3 or LB / RB");
					const ImVec2 buttonSize(300.0f * uiScale * buttonScale, 62.0f * uiScale * buttonScale);
					const auto drawMenuButton = [&](int index, const char* label, int request)
					{
						if (selected == index)
						{
							ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 120, 210, 230));
						}
						if (ImGui::Button(label, buttonSize))
						{
							tran.gameplayDebug.pauseMenuRequest = request;
						}
						if (selected == index)
						{
							ImGui::PopStyleColor();
						}
					};
					drawMenuButton(0, u8"続行", 1);
					drawMenuButton(1, u8"Option", 3);
					drawMenuButton(2, u8"Titleへ", 2);
				}
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::SetWindowFontScale(1.0f);
		ImGui::End();
		ImGui::PopStyleVar(2);
	}
	if (SceneManager::GetCurrent() == SceneManager::SceneType::SCENE_RESULT &&
		!(SceneManager::GetResultType() == SceneManager::ResultType::Win && tran.roguelike.selectionPending != 0))
	{
		ImGuiViewport* vp = ImGui::GetMainViewport();
		ImDrawList* dl = ImGui::GetForegroundDrawList(vp);
		const char* resultGuide =
			u8"リザルト選択\n"
			u8"左: Restart (ゲームへ) / 右: Title (タイトルへ)\n"
			u8"移動: A D / W S / ← → / ↑ ↓\n"
			u8"決定: Enter / F / Space";

		const ImVec2 pad(10.0f, 8.0f);
		const ImVec2 textSize = ImGui::CalcTextSize(resultGuide);
		const ImVec2 boxMin(vp->Pos.x + vp->Size.x - textSize.x - pad.x * 2.0f - 16.0f,
							vp->Pos.y + vp->Size.y - textSize.y - pad.y * 2.0f - 16.0f);
		const ImVec2 boxMax(boxMin.x + textSize.x + pad.x * 2.0f, boxMin.y + textSize.y + pad.y * 2.0f);

		dl->AddRectFilled(boxMin, boxMax, IM_COL32(0, 0, 0, 170), 8.0f);
		dl->AddRect(boxMin, boxMax, IM_COL32(255, 255, 255, 120), 8.0f);
		dl->AddText(ImVec2(boxMin.x + pad.x, boxMin.y + pad.y), IM_COL32(255, 255, 255, 255), resultGuide);
	}
	if (SceneManager::GetCurrent() == SceneManager::SceneType::SCENE_RESULT &&
		tran.gameplayDebug.showBossResultTimer != 0)
	{
		ImGuiViewport* vp = ImGui::GetMainViewport();
		ImDrawList* dl = ImGui::GetForegroundDrawList(vp);
		char timeText[32]{};
		formatRunTime(tran.gameplayDebug.runRecordedSec, timeText, sizeof(timeText));
		const char* timerLabel = u8"記録タイム";
		const char* timerState = u8"TIMER STOP";
		ImFont* font = ImGui::GetFont();
		const float labelSize = ImGui::GetFontSize() * 1.15f;
		const float timeSize = ImGui::GetFontSize() * 3.10f;
		const float stateSize = ImGui::GetFontSize() * 1.00f;
		const ImVec2 labelTextSize = font->CalcTextSizeA(labelSize, 10000.0f, 0.0f, timerLabel);
		const ImVec2 timeTextSize = font->CalcTextSizeA(timeSize, 10000.0f, 0.0f, timeText);
		const ImVec2 stateTextSize = font->CalcTextSizeA(stateSize, 10000.0f, 0.0f, timerState);
		const float contentWidth = (labelTextSize.x > timeTextSize.x)
			? ((labelTextSize.x > stateTextSize.x) ? labelTextSize.x : stateTextSize.x)
			: ((timeTextSize.x > stateTextSize.x) ? timeTextSize.x : stateTextSize.x);
		const float gap = 10.0f;
		const float padX = 28.0f;
		const float padY = 18.0f;
		const float contentHeight = labelTextSize.y + gap + timeTextSize.y + gap + stateTextSize.y;
		const float boxWidth = contentWidth + padX * 2.0f;
		const float boxHeight = contentHeight + padY * 2.0f;
		const ImVec2 boxMin(vp->Pos.x + (vp->Size.x - boxWidth) * 0.5f, vp->Pos.y + 52.0f);
		const ImVec2 boxMax(boxMin.x + boxWidth, boxMin.y + boxHeight);

		dl->AddRectFilled(boxMin, boxMax, IM_COL32(0, 0, 0, 190), 10.0f);
		dl->AddRect(boxMin, boxMax, IM_COL32(255, 255, 255, 180), 10.0f);
		float y = boxMin.y + padY;
		dl->AddText(font, labelSize, ImVec2(boxMin.x + (boxWidth - labelTextSize.x) * 0.5f, y), IM_COL32(230, 230, 230, 255), timerLabel);
		y += labelTextSize.y + gap;
		dl->AddText(font, timeSize, ImVec2(boxMin.x + (boxWidth - timeTextSize.x) * 0.5f, y), IM_COL32(255, 240, 120, 255), timeText);
		y += timeTextSize.y + gap;
		dl->AddText(font, stateSize, ImVec2(boxMin.x + (boxWidth - stateTextSize.x) * 0.5f, y), IM_COL32(210, 210, 210, 255), timerState);
	}
	if (SceneManager::GetCurrent() == SceneManager::SceneType::SCENE_TITLE &&
		tran.gameplayDebug.titleOptionOpen == 0 &&
		tran.gameplayDebug.titleDifficultyOpen == 0)
	{
		ImGuiViewport* vp = ImGui::GetMainViewport();
		ImDrawList* dl = ImGui::GetForegroundDrawList(vp);
		const char* titleGuide =
			u8"タイトル選択\n"
			u8"Start / Option / Exit\n"
			u8"移動: W S A D / ↑ ↓ ← →\n"
			u8"決定: Enter / F / Space\n"
			u8"Esc: 終了";

		const ImVec2 pad(10.0f, 8.0f);
		const ImVec2 textSize = ImGui::CalcTextSize(titleGuide);
		const ImVec2 boxMin(vp->Pos.x + vp->Size.x - textSize.x - pad.x * 2.0f - 16.0f,
							vp->Pos.y + vp->Size.y - textSize.y - pad.y * 2.0f - 16.0f);
		const ImVec2 boxMax(boxMin.x + textSize.x + pad.x * 2.0f, boxMin.y + textSize.y + pad.y * 2.0f);

		dl->AddRectFilled(boxMin, boxMax, IM_COL32(0, 0, 0, 170), 8.0f);
		dl->AddRect(boxMin, boxMax, IM_COL32(255, 255, 255, 120), 8.0f);
		dl->AddText(ImVec2(boxMin.x + pad.x, boxMin.y + pad.y), IM_COL32(255, 255, 255, 255), titleGuide);
	}
	if (SceneManager::GetCurrent() == SceneManager::SceneType::SCENE_TITLE &&
		tran.gameplayDebug.titleOptionOpen != 0)
	{
		const float uiScale = (tran.gameplayDebug.pauseMenuUiScale < 0.5f) ? 0.5f
			: (tran.gameplayDebug.pauseMenuUiScale > 2.5f ? 2.5f : tran.gameplayDebug.pauseMenuUiScale);
		const float fontScale = (tran.gameplayDebug.pauseMenuFontScale < 0.5f) ? 0.5f
			: (tran.gameplayDebug.pauseMenuFontScale > 2.5f ? 2.5f : tran.gameplayDebug.pauseMenuFontScale);
		const int selected = tran.gameplayDebug.titleOptionSelection;
		const bool isFullscreen = IsAppFullscreen();

		ImGuiViewport* vp = ImGui::GetMainViewport();
		const ImVec2 windowSize(560.0f * uiScale, 440.0f * uiScale);
		const ImVec2 windowPos(vp->Pos.x + (vp->Size.x - windowSize.x) * 0.5f,
							   vp->Pos.y + (vp->Size.y - windowSize.y) * 0.5f);
		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18.0f * uiScale, 16.0f * uiScale));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f * uiScale, 12.0f * uiScale));
		ImGui::Begin("##title_option_overlay", nullptr,
					 ImGuiWindowFlags_NoTitleBar |
					 ImGuiWindowFlags_NoCollapse |
					 ImGuiWindowFlags_NoResize |
					 ImGuiWindowFlags_NoMove |
					 ImGuiWindowFlags_NoDocking);
		ImGui::SetWindowFontScale(fontScale);
		ImGui::TextUnformatted(u8"Title - Option");
		const float closeButtonWidth = 88.0f * uiScale;
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - closeButtonWidth);
		if (ImGui::Button("Close##title_option_close", ImVec2(closeButtonWidth, 0.0f)))
		{
			tran.gameplayDebug.titleOptionRequestClose = 1;
		}
		ImGui::Separator();
		const char* selectedLabel = u8"Master";
		switch (selected)
		{
		case 1: selectedLabel = u8"BGM"; break;
		case 2: selectedLabel = u8"SE"; break;
		case 3: selectedLabel = u8"表示"; break;
		case 4: selectedLabel = u8"戻る"; break;
		default: break;
		}
		ImGui::Text(u8"選択中: %s", selectedLabel);

		float master = tran.gameplay.volumeMaster;
		if (ImGui::SliderFloat(u8"Master", &master, 0.0f, 2.0f, "%.2f"))
		{
			tran.gameplay.volumeMaster = master;
		}
		float bgm = tran.gameplay.volumeBgm;
		if (ImGui::SliderFloat(u8"BGM", &bgm, 0.0f, 2.0f, "%.2f"))
		{
			tran.gameplay.volumeBgm = bgm;
		}
		float se = tran.gameplay.volumeSe;
		if (ImGui::SliderFloat(u8"SE", &se, 0.0f, 2.0f, "%.2f"))
		{
			tran.gameplay.volumeSe = se;
		}

		bool fullscreenChecked = isFullscreen;
		bool windowChecked = !isFullscreen;
		if (ImGui::Checkbox(u8"Fullscreen", &fullscreenChecked))
		{
			SetAppFullscreen(fullscreenChecked);
		}
		ImGui::SameLine();
		if (ImGui::Checkbox(u8"Window", &windowChecked))
		{
			SetAppFullscreen(!windowChecked);
		}
		ImGui::Separator();
		ImGui::TextUnformatted(u8"移動: W/S  or  ↑/↓");
		ImGui::TextUnformatted(u8"変更: A/D  or  ←/→");
		ImGui::TextUnformatted(u8"決定: Enter / F / Space");
		ImGui::TextUnformatted(u8"戻る: Esc");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::End();
		ImGui::PopStyleVar(2);
	}

	// DrawList overlay（画面上に線や矩形などを描く）
	// 3D上の座標投影まではやらず、まずは「画面座標の可視化」に寄せてある
	if (show_overlay)
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGuiViewport* vp = ImGui::GetMainViewport();
		ImDrawList* dl = ImGui::GetForegroundDrawList(vp);

		const ImVec2 mouse = io.MousePos;
		const ImVec2 center(vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + vp->Size.y * 0.5f);

		// クロスヘア（マウス）
		const float cross = 10.0f;
		dl->AddLine(ImVec2(mouse.x - cross, mouse.y), ImVec2(mouse.x + cross, mouse.y), IM_COL32(255, 255, 0, 255), 1.0f);
		dl->AddLine(ImVec2(mouse.x, mouse.y - cross), ImVec2(mouse.x, mouse.y + cross), IM_COL32(255, 255, 0, 255), 1.0f);

		// 画面中央マーカー
		dl->AddCircle(center, 6.0f, IM_COL32(0, 255, 255, 255), 16, 1.0f);

		// 右上に簡易HUD（背景付き）
		char hud[256];
		sprintf_s(hud, u8"FPS %.1f\nマウス (%.0f, %.0f)\nプレイヤーHP %.1f / %.1f",
			io.Framerate, mouse.x, mouse.y, tran.player.hp, tran.player.maxHp);

		const ImVec2 pad(8.0f, 6.0f);
		const ImVec2 text_size = ImGui::CalcTextSize(hud);
		const ImVec2 box_min(vp->Pos.x + vp->Size.x - text_size.x - pad.x * 2.0f - 10.0f, vp->Pos.y + 10.0f);
		const ImVec2 box_max(box_min.x + text_size.x + pad.x * 2.0f, box_min.y + text_size.y + pad.y * 2.0f);

		dl->AddRectFilled(box_min, box_max, IM_COL32(0, 0, 0, 160), 4.0f);
		dl->AddRect(box_min, box_max, IM_COL32(255, 255, 255, 100), 4.0f);
		dl->AddText(ImVec2(box_min.x + pad.x, box_min.y + pad.y), IM_COL32(255, 255, 255, 255), hud);
	}

	// ImGui標準デモ（Tables / Docking / Viewports の動作確認用）
	if (show_imgui_demo)
	{
		//ImGui::ShowDemoWindow(&show_imgui_demo);
	}


	if (!IsEngineEditorScene(SceneManager::GetCurrent()))
	{
		// 軸線の表示
		// グリッド
		DirectX::XMFLOAT4 lineColor(0.5f, 0.5f, 0.5f, 1.0f);
		float size = DEBUG_GRID_NUM * DEBUG_GRID_MARGIN;
		for (int i = 1; i <= DEBUG_GRID_NUM; ++i)
		{
			float grid = i * DEBUG_GRID_MARGIN;
			DirectX::XMFLOAT3 pos[2] = {
				DirectX::XMFLOAT3(grid, 0.0f, size),
				DirectX::XMFLOAT3(grid, 0.0f,-size),
			};
			Geometory::AddLine(pos[0], pos[1], lineColor);
			pos[0].x = pos[1].x = -grid;
			Geometory::AddLine(pos[0], pos[1], lineColor);
			pos[0].x = size;
			pos[1].x = -size;
			pos[0].z = pos[1].z = grid;
			Geometory::AddLine(pos[0], pos[1], lineColor);
			pos[0].z = pos[1].z = -grid;
			Geometory::AddLine(pos[0], pos[1], lineColor);
		}
		// 軸
		Geometory::AddLine(DirectX::XMFLOAT3(0,0,0), DirectX::XMFLOAT3(size,0,0), DirectX::XMFLOAT4(1,0,0,1));
		Geometory::AddLine(DirectX::XMFLOAT3(0,0,0), DirectX::XMFLOAT3(0,size,0), DirectX::XMFLOAT4(0,1,0,1));
		Geometory::AddLine(DirectX::XMFLOAT3(0,0,0), DirectX::XMFLOAT3(0,0,size), DirectX::XMFLOAT4(0,0,1,1));
		Geometory::AddLine(DirectX::XMFLOAT3(0,0,0), DirectX::XMFLOAT3(-size,0,0),  DirectX::XMFLOAT4(0,0,0,1));
		Geometory::AddLine(DirectX::XMFLOAT3(0,0,0), DirectX::XMFLOAT3(0,0,-size),  DirectX::XMFLOAT4(0,0,0,1));

		Geometory::DrawLines();

		// カメラの値
		static bool camAutoSwitch = false;
		static bool camUpDownSwitch = true;
		static float camAutoRotate = 1.0f;
		if (IsKeyTrigger(VK_RETURN) && false) {
			camAutoSwitch ^= true;
		}
		if (IsKeyTrigger(VK_SPACE)) {
			camUpDownSwitch ^= true;
		}

		DirectX::XMVECTOR camPos;
		if (camAutoSwitch) {
			camAutoRotate += 0.01f;
		}
		camPos = DirectX::XMVectorSet(
			cosf(camAutoRotate) * 5.0f,
			3.5f * (camUpDownSwitch ? 1.0f : -1.0f),
			sinf(camAutoRotate) * 5.0f,
			0.0f);

		// ジオメトリ用カメラ初期化
		DirectX::XMFLOAT4X4 mat[2];
		DirectX::XMStoreFloat4x4(&mat[0], DirectX::XMMatrixTranspose(
			DirectX::XMMatrixLookAtLH(
				camPos,
				DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
				DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
			)));
		DirectX::XMStoreFloat4x4(&mat[1], DirectX::XMMatrixTranspose(
			DirectX::XMMatrixPerspectiveFovLH(
				DirectX::XMConvertToRadians(60.0f), (float)SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 1000.0f)
		));
		Geometory::SetView(mat[0]);
		Geometory::SetProjection(mat[1]);
	}
#endif

	if (IsEngineEditorScene(SceneManager::GetCurrent()))
	{
		DrawSceneToEngineEditorRenderTarget();
	}
	else
	{
		SceneManager::Draw();
	}
	{
		TRAN_INS;
#ifdef _DEBUG
		const bool showBossDebugHint = true;
#else
		const bool showBossDebugHint = false;
#endif
		DrawUpgradeSelectionOverlay(tran, showBossDebugHint);
	}
#ifndef _DEBUG
	{
		TRAN_INS;
		if (SceneManager::GetCurrent() == SceneManager::SceneType::SCENE_TITLE &&
			tran.gameplayDebug.titleOptionOpen == 0 &&
			tran.gameplayDebug.titleDifficultyOpen == 0)
		{
			ImGuiViewport* vp = ImGui::GetMainViewport();
			ImDrawList* dl = ImGui::GetForegroundDrawList(vp);
			const char* titleGuide =
				u8"タイトル選択\n"
				u8"Start / Option / Exit\n"
				u8"移動: W S A D / ↑ ↓ ← →\n"
				u8"決定: Enter / F / Space\n"
				u8"Esc: 終了";

			const ImVec2 pad(10.0f, 8.0f);
			const ImVec2 textSize = ImGui::CalcTextSize(titleGuide);
			const ImVec2 boxMin(vp->Pos.x + vp->Size.x - textSize.x - pad.x * 2.0f - 16.0f,
								vp->Pos.y + vp->Size.y - textSize.y - pad.y * 2.0f - 16.0f);
			const ImVec2 boxMax(boxMin.x + textSize.x + pad.x * 2.0f, boxMin.y + textSize.y + pad.y * 2.0f);

			dl->AddRectFilled(boxMin, boxMax, IM_COL32(0, 0, 0, 170), 8.0f);
			dl->AddRect(boxMin, boxMax, IM_COL32(255, 255, 255, 120), 8.0f);
			dl->AddText(ImVec2(boxMin.x + pad.x, boxMin.y + pad.y), IM_COL32(255, 255, 255, 255), titleGuide);
		}
	}
#endif
	EndDrawDirectX();
}

// EOF

