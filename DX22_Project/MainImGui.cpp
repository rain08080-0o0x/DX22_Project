#include "MainImGui.h"

#include <Windows.h>
#include <commdlg.h>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <cstdio>

#include "CastleSaveData.h"
#include "DirectX.h"
#include "Main.h"
#include "SceneCastleEditor.h"
#include "Texture.h"
#include "Transfer.h"
#include "imgui.h"

#undef max
#undef min

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

const char* GetRunStageTypeName(int stageType)
{
	switch (stageType)
	{
	case Transfer::RoguelikeUpgrade::StageShop:
		return u8"ショップ";
	case Transfer::RoguelikeUpgrade::StageRest:
		return u8"休憩所";
	case Transfer::RoguelikeUpgrade::StageBoss:
		return u8"ボス";
	case Transfer::RoguelikeUpgrade::StageFinalBoss:
		return u8"ラスボス";
	default:
		return u8"戦闘";
	}
}

const char* GetRunRewardTypeName(int rewardType)
{
	switch (rewardType)
	{
	case Transfer::RoguelikeUpgrade::RewardSkill:
		return u8"魔法強化";
	case Transfer::RoguelikeUpgrade::RewardTrait:
		return u8"特性強化";
	case Transfer::RoguelikeUpgrade::RewardTag:
		return u8"タグ解除";
	case Transfer::RoguelikeUpgrade::RewardWeapon:
		return u8"武器強化";
	case Transfer::RoguelikeUpgrade::RewardDice:
		return u8"サイコロ";
	case Transfer::RoguelikeUpgrade::RewardArtifact:
		return u8"魔道具";
	case Transfer::RoguelikeUpgrade::RewardShop:
		return u8"ショップ";
	case Transfer::RoguelikeUpgrade::RewardRest:
		return u8"休憩所";
	case Transfer::RoguelikeUpgrade::RewardBoss:
		return u8"ボス";
	case Transfer::RoguelikeUpgrade::RewardFinalBoss:
		return u8"ラスボス";
	default:
		return u8"未定";
	}
}

namespace
{
	struct BindingOption
	{
		int value;
		const char* label;
	};

	struct BindingRow
	{
		const char* label;
		int Transfer::InputConfig::*member;
	};

	float ClampFloat(float value, float minValue, float maxValue)
	{
		if (value < minValue) return minValue;
		if (value > maxValue) return maxValue;
		return value;
	}

	ImVec2 FitOverlayWindowSize(const ImGuiViewport* viewport, const ImVec2& desiredSize)
	{
		if (!viewport)
		{
			return desiredSize;
		}

		ImVec2 clampedSize = desiredSize;
		const float maxWidth = viewport->Size.x * 0.92f;
		const float maxHeight = viewport->Size.y * 0.90f;
		if (clampedSize.x > maxWidth) clampedSize.x = maxWidth;
		if (clampedSize.y > maxHeight) clampedSize.y = maxHeight;
		return clampedSize;
	}

	float ComputeAdaptiveFactor(const ImGuiViewport* viewport,
								float weight,
								bool isFullscreen,
								float fullscreenBoost,
								float windowBoost)
	{
		if (!viewport)
		{
			return 1.0f;
		}

		float viewportScale = viewport->Size.x / 1280.0f;
		const float heightScale = viewport->Size.y / 720.0f;
		if (heightScale < viewportScale)
		{
			viewportScale = heightScale;
		}
		viewportScale = ClampFloat(viewportScale, 0.70f, 1.80f);

		float factor = 1.0f + (viewportScale - 1.0f) * weight;
		factor *= isFullscreen ? fullscreenBoost : windowBoost;
		return ClampFloat(factor, 0.80f, 1.60f);
	}

	const BindingOption kKeyboardBindingOptions[] =
	{
		{ 'A', "A" }, { 'B', "B" }, { 'C', "C" }, { 'D', "D" }, { 'E', "E" }, { 'F', "F" },
		{ 'G', "G" }, { 'H', "H" }, { 'I', "I" }, { 'J', "J" }, { 'K', "K" }, { 'L', "L" },
		{ 'M', "M" }, { 'N', "N" }, { 'O', "O" }, { 'P', "P" }, { 'Q', "Q" }, { 'R', "R" },
		{ 'S', "S" }, { 'T', "T" }, { 'U', "U" }, { 'V', "V" }, { 'W', "W" }, { 'X', "X" },
		{ 'Y', "Y" }, { 'Z', "Z" },
		{ '0', "0" }, { '1', "1" }, { '2', "2" }, { '3', "3" }, { '4', "4" },
		{ '5', "5" }, { '6', "6" }, { '7', "7" }, { '8', "8" }, { '9', "9" },
		{ VK_UP, "Up Arrow" }, { VK_DOWN, "Down Arrow" }, { VK_LEFT, "Left Arrow" }, { VK_RIGHT, "Right Arrow" },
		{ VK_SPACE, "Space" }, { VK_RETURN, "Enter" }, { VK_SHIFT, "Shift" }, { VK_TAB, "Tab" }, { VK_CONTROL, "Ctrl" }
	};

	const BindingOption kXInputBindingOptions[] =
	{
		{ Transfer::InputConfig::XInputA, "A" },
		{ Transfer::InputConfig::XInputB, "B" },
		{ Transfer::InputConfig::XInputX, "X" },
		{ Transfer::InputConfig::XInputY, "Y" },
		{ Transfer::InputConfig::XInputLeftShoulder, "Left Shoulder" },
		{ Transfer::InputConfig::XInputRightShoulder, "Right Shoulder" },
		{ Transfer::InputConfig::XInputBack, "Back" },
		{ Transfer::InputConfig::XInputLeftThumb, "Left Thumb" },
		{ Transfer::InputConfig::XInputRightThumb, "Right Thumb" }
	};

	const BindingRow kKeyboardBindingRows[] =
	{
		{ "Move Up", &Transfer::InputConfig::moveUp },
		{ "Move Down", &Transfer::InputConfig::moveDown },
		{ "Move Left", &Transfer::InputConfig::moveLeft },
		{ "Move Right", &Transfer::InputConfig::moveRight },
		{ "Dash", &Transfer::InputConfig::dash },
		{ "Attack", &Transfer::InputConfig::attack },
		{ "Skill 1", &Transfer::InputConfig::skill1 },
		{ "Skill 2", &Transfer::InputConfig::skill2 },
		{ "Reroll", &Transfer::InputConfig::reroll }
	};

	const BindingRow kXInputBindingRows[] =
	{
		{ "Confirm", &Transfer::InputConfig::xinputConfirm },
		{ "Dash", &Transfer::InputConfig::xinputDash },
		{ "Attack", &Transfer::InputConfig::xinputAttack },
		{ "Skill 1", &Transfer::InputConfig::xinputSkill1 },
		{ "Skill 2", &Transfer::InputConfig::xinputSkill2 },
		{ "Reroll", &Transfer::InputConfig::xinputReroll },
		{ "Prev Tab", &Transfer::InputConfig::xinputTabPrev },
		{ "Next Tab", &Transfer::InputConfig::xinputTabNext }
	};

	const BindingRow kDirectInputBindingRows[] =
	{
		{ "Confirm", &Transfer::InputConfig::directInputConfirm },
		{ "Dash", &Transfer::InputConfig::directInputDash },
		{ "Attack", &Transfer::InputConfig::directInputAttack },
		{ "Skill 1", &Transfer::InputConfig::directInputSkill1 },
		{ "Skill 2", &Transfer::InputConfig::directInputSkill2 },
		{ "Reroll", &Transfer::InputConfig::directInputReroll },
		{ "Prev Tab", &Transfer::InputConfig::directInputTabPrev },
		{ "Next Tab", &Transfer::InputConfig::directInputTabNext }
	};

	const char* FindBindingLabel(const BindingOption* options, size_t optionCount, int value)
	{
		for (size_t i = 0; i < optionCount; ++i)
		{
			if (options[i].value == value)
			{
				return options[i].label;
			}
		}
		return "Unknown";
	}

	bool DrawBindingCombo(const char* comboId, int* value, const BindingOption* options, size_t optionCount)
	{
		if (!value)
		{
			return false;
		}

		bool changed = false;
		const char* preview = FindBindingLabel(options, optionCount, *value);
		if (ImGui::BeginCombo(comboId, preview))
		{
			for (size_t i = 0; i < optionCount; ++i)
			{
				const bool selected = (*value == options[i].value);
				if (ImGui::Selectable(options[i].label, selected))
				{
					*value = options[i].value;
					changed = true;
				}
				if (selected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		return changed;
	}

	bool DrawDirectInputBindingCombo(const char* comboId, int* value)
	{
		if (!value)
		{
			return false;
		}

		bool changed = false;
		char preview[32]{};
		sprintf_s(preview, sizeof(preview), "Button %d", *value);
		if (ImGui::BeginCombo(comboId, preview))
		{
			for (int buttonIndex = 0; buttonIndex <= 31; ++buttonIndex)
			{
				char label[32]{};
				sprintf_s(label, sizeof(label), "Button %d", buttonIndex);
				const bool selected = (*value == buttonIndex);
				if (ImGui::Selectable(label, selected))
				{
					*value = buttonIndex;
					changed = true;
				}
				if (selected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		return changed;
	}

	bool DrawBindingTable(const char* tableId,
						  const BindingRow* rows,
						  size_t rowCount,
						  Transfer::InputConfig& input,
						  const BindingOption* options,
						  size_t optionCount)
	{
		bool changed = false;
		const ImGuiTableFlags flags =
			ImGuiTableFlags_BordersInnerV |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_SizingStretchProp;
		if (!ImGui::BeginTable(tableId, 2, flags))
		{
			return false;
		}

		ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 0.44f);
		ImGui::TableSetupColumn("Binding", ImGuiTableColumnFlags_WidthStretch, 0.56f);
		ImGui::TableHeadersRow();
		for (size_t rowIndex = 0; rowIndex < rowCount; ++rowIndex)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(rows[rowIndex].label);
			ImGui::TableSetColumnIndex(1);
			ImGui::PushID(static_cast<int>(rowIndex));
			ImGui::SetNextItemWidth(-1.0f);
			changed |= DrawBindingCombo("##binding", &(input.*(rows[rowIndex].member)), options, optionCount);
			ImGui::PopID();
		}

		ImGui::EndTable();
		return changed;
	}

	bool DrawDirectInputBindingTable(const char* tableId,
									 const BindingRow* rows,
									 size_t rowCount,
									 Transfer::InputConfig& input)
	{
		bool changed = false;
		const ImGuiTableFlags flags =
			ImGuiTableFlags_BordersInnerV |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_SizingStretchProp;
		if (!ImGui::BeginTable(tableId, 2, flags))
		{
			return false;
		}

		ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 0.44f);
		ImGui::TableSetupColumn("Binding", ImGuiTableColumnFlags_WidthStretch, 0.56f);
		ImGui::TableHeadersRow();
		for (size_t rowIndex = 0; rowIndex < rowCount; ++rowIndex)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(rows[rowIndex].label);
			ImGui::TableSetColumnIndex(1);
			ImGui::PushID(static_cast<int>(rowIndex));
			ImGui::SetNextItemWidth(-1.0f);
			changed |= DrawDirectInputBindingCombo("##binding", &(input.*(rows[rowIndex].member)));
			ImGui::PopID();
		}

		ImGui::EndTable();
		return changed;
	}

	void ResetKeyboardBindings(Transfer::InputConfig& input)
	{
		const Transfer::InputConfig defaults{};
		input.moveUp = defaults.moveUp;
		input.moveDown = defaults.moveDown;
		input.moveLeft = defaults.moveLeft;
		input.moveRight = defaults.moveRight;
		input.dash = defaults.dash;
		input.attack = defaults.attack;
		input.skill1 = defaults.skill1;
		input.skill2 = defaults.skill2;
		input.reroll = defaults.reroll;
	}

	void ResetXInputBindings(Transfer::InputConfig& input)
	{
		const Transfer::InputConfig defaults{};
		input.xinputConfirm = defaults.xinputConfirm;
		input.xinputDash = defaults.xinputDash;
		input.xinputAttack = defaults.xinputAttack;
		input.xinputSkill1 = defaults.xinputSkill1;
		input.xinputSkill2 = defaults.xinputSkill2;
		input.xinputReroll = defaults.xinputReroll;
		input.xinputTabPrev = defaults.xinputTabPrev;
		input.xinputTabNext = defaults.xinputTabNext;
	}

	void ResetDirectInputBindings(Transfer::InputConfig& input)
	{
		const Transfer::InputConfig defaults{};
		input.directInputConfirm = defaults.directInputConfirm;
		input.directInputDash = defaults.directInputDash;
		input.directInputAttack = defaults.directInputAttack;
		input.directInputSkill1 = defaults.directInputSkill1;
		input.directInputSkill2 = defaults.directInputSkill2;
		input.directInputReroll = defaults.directInputReroll;
		input.directInputTabPrev = defaults.directInputTabPrev;
		input.directInputTabNext = defaults.directInputTabNext;
	}
}

OverlayScaleMetrics GetAdaptiveOverlayScaleMetrics(const Transfer& tran, bool isFullscreen)
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const float userUiScale = ClampFloat(tran.gameplayDebug.pauseMenuUiScale, 0.5f, 2.5f);
	const float userFontScale = ClampFloat(tran.gameplayDebug.pauseMenuFontScale, 0.5f, 2.5f);
	const float userButtonScale = ClampFloat(tran.gameplayDebug.pauseMenuButtonScale, 0.5f, 2.5f);

	OverlayScaleMetrics metrics{};
	metrics.uiScale = ClampFloat(
		userUiScale * ComputeAdaptiveFactor(viewport, 0.35f, isFullscreen, 1.08f, 0.96f),
		0.50f,
		3.20f);
	metrics.fontScale = ClampFloat(
		userFontScale * ComputeAdaptiveFactor(viewport, 0.45f, isFullscreen, 1.10f, 0.98f),
		0.50f,
		3.40f);
	metrics.buttonScale = ClampFloat(
		userButtonScale * ComputeAdaptiveFactor(viewport, 0.28f, isFullscreen, 1.05f, 0.98f),
		0.50f,
		3.20f);
	return metrics;
}

void DrawTitleOptionOverlay(Transfer& tran)
{
	if (SceneManager::GetCurrent() != SceneManager::SceneType::SCENE_TITLE) return;
	if (tran.gameplayDebug.titleOptionOpen == 0) return;
	if (tran.gameplayDebug.titleKeyConfigOpen != 0) return;

	const bool isFullscreen = IsAppFullscreen();
	const OverlayScaleMetrics metrics = GetAdaptiveOverlayScaleMetrics(tran, isFullscreen);
	const int selected = tran.gameplayDebug.titleOptionSelection;
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	if (!viewport)
	{
		return;
	}

	const ImVec2 baseSize(560.0f * metrics.uiScale, 440.0f * metrics.uiScale);
	const ImVec2 windowSize = FitOverlayWindowSize(viewport, baseSize);
	const ImVec2 windowPos(
		viewport->Pos.x + (viewport->Size.x - windowSize.x) * 0.5f,
		viewport->Pos.y + (viewport->Size.y - windowSize.y) * 0.5f);

	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f * metrics.uiScale, 18.0f * metrics.uiScale));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f * metrics.uiScale, 13.0f * metrics.uiScale));
	ImGui::Begin("##title_option_overlay", nullptr,
				 ImGuiWindowFlags_NoTitleBar |
				 ImGuiWindowFlags_NoCollapse |
				 ImGuiWindowFlags_NoResize |
				 ImGuiWindowFlags_NoMove |
				 ImGuiWindowFlags_NoDocking);
	ImGui::SetWindowFontScale(metrics.fontScale);
	ImGui::TextUnformatted(u8"Title - Option");
	const float closeButtonWidth = 94.0f * metrics.uiScale;
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
	case 4: selectedLabel = u8"キーコンフィグ"; break;
	case 5: selectedLabel = u8"エフェクト確認"; break;
	case 6: selectedLabel = u8"戻る"; break;
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

	const bool fullscreenSelected = isFullscreen;
	const bool windowSelected = !isFullscreen;
	if (ImGui::RadioButton(u8"Fullscreen", fullscreenSelected))
	{
		SetAppFullscreen(true);
	}
	ImGui::SameLine();
	if (ImGui::RadioButton(u8"Window", windowSelected))
	{
		SetAppFullscreen(false);
	}

	if (selected == 4)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 120, 210, 230));
	}
	if (ImGui::Button(u8"キーコンフィグ", ImVec2(220.0f * metrics.uiScale, 0.0f)))
	{
		tran.gameplayDebug.titleKeyConfigRequestOpen = 1;
	}
	if (selected == 4)
	{
		ImGui::PopStyleColor();
	}

	if (selected == 5)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 140, 90, 230));
	}
	if (ImGui::Button(u8"エフェクト確認", ImVec2(220.0f * metrics.uiScale, 0.0f)))
	{
		tran.gameplayDebug.titleOptionOpen = 0;
		tran.gameplayDebug.titleOptionSelection = 0;
		tran.gameplayDebug.titleOptionRequestClose = 0;
		tran.gameplayDebug.titleKeyConfigOpen = 0;
		tran.gameplayDebug.titleKeyConfigRequestOpen = 0;
		SceneManager::ChangeScene(SceneManager::SCENE_EFFECT_DEBUG);
	}
	if (selected == 5)
	{
		ImGui::PopStyleColor();
	}

	ImGui::Separator();
	ImGui::TextUnformatted(u8"移動: W/S or Up/Down");
	ImGui::TextUnformatted(u8"変更: A/D or Left/Right");
	ImGui::TextUnformatted(u8"決定: Enter / F / Space / Controller Confirm");
	ImGui::TextUnformatted(u8"戻る: Esc / Start / DirectInput Button 9");
	ImGui::SetWindowFontScale(1.0f);
	ImGui::End();
	ImGui::PopStyleVar(2);
}

void DrawTitleKeyConfigWindow(Transfer& tran)
{
	if (SceneManager::GetCurrent() != SceneManager::SceneType::SCENE_TITLE) return;
	if (tran.gameplayDebug.titleKeyConfigOpen == 0) return;

	const bool isFullscreen = IsAppFullscreen();
	const OverlayScaleMetrics metrics = GetAdaptiveOverlayScaleMetrics(tran, isFullscreen);
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	if (!viewport)
	{
		return;
	}

	bool changed = false;
	const ImVec2 baseSize(860.0f * metrics.uiScale, 640.0f * metrics.uiScale);
	const ImVec2 windowSize = FitOverlayWindowSize(viewport, baseSize);
	const ImVec2 windowPos(
		viewport->Pos.x + (viewport->Size.x - windowSize.x) * 0.5f,
		viewport->Pos.y + (viewport->Size.y - windowSize.y) * 0.5f);

	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f * metrics.uiScale, 18.0f * metrics.uiScale));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f * metrics.uiScale, 12.0f * metrics.uiScale));
	ImGui::Begin("##title_keyconfig_overlay", nullptr,
				 ImGuiWindowFlags_NoTitleBar |
				 ImGuiWindowFlags_NoCollapse |
				 ImGuiWindowFlags_NoResize |
				 ImGuiWindowFlags_NoMove |
				 ImGuiWindowFlags_NoDocking);
	ImGui::SetWindowFontScale(metrics.fontScale);
	ImGui::TextUnformatted(u8"Title - Key Config");
	ImGui::Separator();
	ImGui::TextDisabled(u8"Esc / Start / DirectInput Button 9 は固定です。");
	ImGui::TextDisabled(u8"設定は変更した瞬間に保存されます。");
	ImGui::Spacing();

	if (ImGui::BeginTabBar("##title_keyconfig_tabs", ImGuiTabBarFlags_FittingPolicyResizeDown))
	{
		if (ImGui::BeginTabItem("Keyboard"))
		{
			ImGui::TextDisabled(u8"キーボードのみ変更できます。Move / Dash / Attack / Skill / Reroll を設定します。");
			changed |= DrawBindingTable(
				"##keyboard_binding_table",
				kKeyboardBindingRows,
				sizeof(kKeyboardBindingRows) / sizeof(kKeyboardBindingRows[0]),
				tran.input,
				kKeyboardBindingOptions,
				sizeof(kKeyboardBindingOptions) / sizeof(kKeyboardBindingOptions[0]));
			if (ImGui::Button(u8"キーボードを初期化", ImVec2(220.0f * metrics.buttonScale, 0.0f)))
			{
				ResetKeyboardBindings(tran.input);
				changed = true;
			}
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Controller"))
		{
			ImGui::TextDisabled(u8"Move は LeftStick / DPad 固定です。Back / Pause は Start または DirectInput Button 9 固定です。");
			if (ImGui::BeginTabBar("##controller_binding_tabs", ImGuiTabBarFlags_FittingPolicyResizeDown))
			{
				if (ImGui::BeginTabItem("XInput"))
				{
					changed |= DrawBindingTable(
						"##xinput_binding_table",
						kXInputBindingRows,
						sizeof(kXInputBindingRows) / sizeof(kXInputBindingRows[0]),
						tran.input,
						kXInputBindingOptions,
						sizeof(kXInputBindingOptions) / sizeof(kXInputBindingOptions[0]));
					if (ImGui::Button(u8"XInputを初期化", ImVec2(220.0f * metrics.buttonScale, 0.0f)))
					{
						ResetXInputBindings(tran.input);
						changed = true;
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("DirectInput"))
				{
					ImGui::TextDisabled(u8"DirectInput は Button 0 から 31 の範囲で設定します。");
					changed |= DrawDirectInputBindingTable(
						"##directinput_binding_table",
						kDirectInputBindingRows,
						sizeof(kDirectInputBindingRows) / sizeof(kDirectInputBindingRows[0]),
						tran.input);
					if (ImGui::Button(u8"DirectInputを初期化", ImVec2(220.0f * metrics.buttonScale, 0.0f)))
					{
						ResetDirectInputBindings(tran.input);
						changed = true;
					}
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	ImGui::Separator();
	if (ImGui::Button(u8"すべて初期化", ImVec2(220.0f * metrics.buttonScale, 0.0f)))
	{
		tran.ResetInputConfigToDefault();
		changed = true;
	}
	ImGui::SameLine();
	ImGui::TextDisabled(u8"戻る: Esc / Start / DirectInput Button 9");

	if (changed)
	{
		tran.SaveGameplayTuning();
	}

	ImGui::SetWindowFontScale(1.0f);
	ImGui::End();
	ImGui::PopStyleVar(2);
}

static bool BrowseCastleEditorModelPath(char* outPath, size_t outPathSize)
{
	if (!outPath || outPathSize == 0) return false;

	OPENFILENAMEA ofn{};
	char filePath[MAX_PATH] = "";
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = GetForegroundWindow();
	ofn.lpstrFile = filePath;
	ofn.nMaxFile = static_cast<DWORD>(sizeof(filePath));
	ofn.lpstrFilter =
		"3D Model Files (*.fbx;*.obj;*.gltf;*.glb;*.pmx;*.pmd)\0*.fbx;*.obj;*.gltf;*.glb;*.pmx;*.pmd\0"
		"FBX Files (*.fbx)\0*.fbx\0"
		"OBJ Files (*.obj)\0*.obj\0"
		"glTF Files (*.gltf;*.glb)\0*.gltf;*.glb\0"
		"MMD Model Files (*.pmx;*.pmd)\0*.pmx;*.pmd\0"
		"All Files (*.*)\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	ofn.lpstrDefExt = "fbx";

	if (!GetOpenFileNameA(&ofn))
	{
		return false;
	}

	strcpy_s(outPath, outPathSize, filePath);
	return true;
}

namespace
{
	int GetPackedOfferType(int packedOffer)
	{
		if (packedOffer < 0)
		{
			return Transfer::RoguelikeUpgrade::OfferNone;
		}
		return packedOffer / Transfer::RoguelikeUpgrade::kOfferTypeStride;
	}

	int GetPackedOfferPrimary(int packedOffer)
	{
		if (packedOffer < 0)
		{
			return 0;
		}
		return (packedOffer % Transfer::RoguelikeUpgrade::kOfferTypeStride) /
			Transfer::RoguelikeUpgrade::kOfferPrimaryStride;
	}

	int GetPackedOfferSecondary(int packedOffer)
	{
		if (packedOffer < 0)
		{
			return 0;
		}
		return packedOffer % Transfer::RoguelikeUpgrade::kOfferPrimaryStride;
	}

	const char* GetActionSkillLabelLocal(int skillType)
	{
		switch (skillType)
		{
		case Transfer::RoguelikeUpgrade::ActionSkillWhirl:
			return u8"薙ぎ払い";
		case Transfer::RoguelikeUpgrade::ActionSkillRush:
			return u8"突進";
		case Transfer::RoguelikeUpgrade::ActionSkillAmbush:
			return u8"奇襲";
		case Transfer::RoguelikeUpgrade::ActionSkillChainThrow:
			return u8"鎖投げ";
		case Transfer::RoguelikeUpgrade::ActionSkillFireball:
			return u8"火球";
		case Transfer::RoguelikeUpgrade::ActionSkillBloodSlash:
			return u8"出血斬";
		default:
			return u8"未設定";
		}
	}

	const char* GetTraitLabelLocal(int traitType)
	{
		switch (traitType)
		{
		case Transfer::RoguelikeUpgrade::TraitCooldown:
			return u8"CT";
		case Transfer::RoguelikeUpgrade::TraitWeapon:
			return u8"武器";
		case Transfer::RoguelikeUpgrade::TraitChain:
			return u8"鎖";
		case Transfer::RoguelikeUpgrade::TraitBlood:
			return u8"血";
		case Transfer::RoguelikeUpgrade::TraitFire:
			return u8"炎";
		default:
			return u8"不明";
		}
	}

	const char* GetWeaponUpgradeLabelLocal(int weaponUpgradeType)
	{
		switch (weaponUpgradeType)
		{
		case Transfer::RoguelikeUpgrade::WeaponUpgradeCooldownBurst:
			return u8"CRT時 CD短縮";
		case Transfer::RoguelikeUpgrade::WeaponUpgradeChainOnCrit:
			return u8"CRT時 鎖付与";
		case Transfer::RoguelikeUpgrade::WeaponUpgradeBloodOnCrit:
			return u8"CRT時 血生成";
		case Transfer::RoguelikeUpgrade::WeaponUpgradeFireOnCrit:
			return u8"CRT時 燃焼";
		case Transfer::RoguelikeUpgrade::WeaponUpgradeCritNeedReduce:
			return u8"CRT必要回数 -1";
		default:
			return u8"不明";
		}
	}

	const char* GetSkillEnhancementLabelLocal(int enhancementType)
	{
		switch (enhancementType)
		{
		case Transfer::RoguelikeUpgrade::SkillEnhanceCooldown:
			return u8"CT強化";
		case Transfer::RoguelikeUpgrade::SkillEnhanceWeapon:
			return u8"武器強化";
		case Transfer::RoguelikeUpgrade::SkillEnhanceChain:
			return u8"鎖強化";
		case Transfer::RoguelikeUpgrade::SkillEnhanceBlood:
			return u8"血強化";
		case Transfer::RoguelikeUpgrade::SkillEnhanceFire:
			return u8"炎強化";
		default:
			return u8"不明";
		}
	}

	const char* GetArtifactLabelLocal(int artifactType)
	{
		switch (artifactType)
		{
		case Transfer::RoguelikeUpgrade::ArtifactBarbarianNecklace:
			return u8"蛮族の首飾り";
		case Transfer::RoguelikeUpgrade::ArtifactGreatShieldCrest:
			return u8"大盾の紋章";
		case Transfer::RoguelikeUpgrade::ArtifactGlassShoes:
			return u8"ガラスの靴";
		case Transfer::RoguelikeUpgrade::ArtifactMagicPiggyBank:
			return u8"魔法の貯金箱";
		case Transfer::RoguelikeUpgrade::ArtifactDiceBox:
			return u8"サイコロのでる箱";
		default:
			return u8"不明";
		}
	}
}

void FormatUpgradeLabel(const Transfer& tran, int upgradeType, char* out, size_t outSize)
{
	if (!out || outSize == 0) return;

	if (upgradeType >= Transfer::RoguelikeUpgrade::kOfferTypeStride)
	{
		const int offerType = GetPackedOfferType(upgradeType);
		const int primaryValue = GetPackedOfferPrimary(upgradeType);
		const int secondaryValue = GetPackedOfferSecondary(upgradeType);
		switch (offerType)
		{
		case Transfer::RoguelikeUpgrade::OfferTrait:
			sprintf_s(out, outSize, u8"%s特性", GetTraitLabelLocal(primaryValue));
			return;
		case Transfer::RoguelikeUpgrade::OfferWeaponUpgrade:
			sprintf_s(out, outSize, u8"武器: %s", GetWeaponUpgradeLabelLocal(primaryValue));
			return;
		case Transfer::RoguelikeUpgrade::OfferSkillEnhance:
			sprintf_s(
				out,
				outSize,
				u8"%s: %s",
				GetActionSkillLabelLocal(primaryValue),
				GetSkillEnhancementLabelLocal(secondaryValue));
			return;
		case Transfer::RoguelikeUpgrade::OfferSkillChange:
			sprintf_s(out, outSize, u8"スキル%d変更 -> %s", primaryValue + 1, GetActionSkillLabelLocal(secondaryValue));
			return;
		case Transfer::RoguelikeUpgrade::OfferTagDisable:
			sprintf_s(out, outSize, u8"タグ解除: %s", GetTraitLabelLocal(primaryValue));
			return;
		case Transfer::RoguelikeUpgrade::OfferArtifact:
			sprintf_s(out, outSize, u8"魔道具: %s", GetArtifactLabelLocal(primaryValue));
			return;
		default:
			sprintf_s(out, outSize, "%s", u8"ここには何もないようだ");
			return;
		}
	}

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

static void FormatUpgradeDescription(const Transfer& tran, int upgradeType, char* out, size_t outSize)
{
	if (!out || outSize == 0) return;

	if (upgradeType >= Transfer::RoguelikeUpgrade::kOfferTypeStride)
	{
		const int offerType = GetPackedOfferType(upgradeType);
		const int primaryValue = GetPackedOfferPrimary(upgradeType);
		const int secondaryValue = GetPackedOfferSecondary(upgradeType);
		switch (offerType)
		{
		case Transfer::RoguelikeUpgrade::OfferTrait:
			switch (primaryValue)
			{
			case Transfer::RoguelikeUpgrade::TraitCooldown:
				sprintf_s(out, outSize, u8"スキル全体CT -5%% / 偶数Lvで追加効果");
				break;
			case Transfer::RoguelikeUpgrade::TraitWeapon:
				sprintf_s(out, outSize, u8"武器倍率 +10%% / 偶数Lvで追加効果");
				break;
			case Transfer::RoguelikeUpgrade::TraitChain:
				sprintf_s(out, outSize, u8"鎖1つにつきHP上限の5%%を縛る");
				break;
			case Transfer::RoguelikeUpgrade::TraitBlood:
				sprintf_s(out, outSize, u8"血ストック上限 +2");
				break;
			case Transfer::RoguelikeUpgrade::TraitFire:
				sprintf_s(out, outSize, u8"燃焼量 +10%%");
				break;
			default:
				sprintf_s(out, outSize, u8"特性を1段階強化");
				break;
			}
			return;
		case Transfer::RoguelikeUpgrade::OfferWeaponUpgrade:
			sprintf_s(out, outSize, u8"武器強化を1つ取得 重複なし 最大4個");
			return;
		case Transfer::RoguelikeUpgrade::OfferSkillEnhance:
			sprintf_s(
				out,
				outSize,
				u8"%s に %s を追加",
				GetActionSkillLabelLocal(primaryValue),
				GetSkillEnhancementLabelLocal(secondaryValue));
			return;
		case Transfer::RoguelikeUpgrade::OfferSkillChange:
			sprintf_s(
				out,
				outSize,
				u8"スキル%dを %s に交換 強化内容も引き継ぐ",
				primaryValue + 1,
				GetActionSkillLabelLocal(secondaryValue));
			return;
		case Transfer::RoguelikeUpgrade::OfferTagDisable:
			sprintf_s(out, outSize, u8"今後 %s 系の候補を出さない", GetTraitLabelLocal(primaryValue));
			return;
		case Transfer::RoguelikeUpgrade::OfferArtifact:
			switch (primaryValue)
			{
			case Transfer::RoguelikeUpgrade::ArtifactBarbarianNecklace:
				sprintf_s(out, outSize, u8"与ダメ+25%% / 被ダメ+30%%");
				break;
			case Transfer::RoguelikeUpgrade::ArtifactGreatShieldCrest:
				sprintf_s(out, outSize, u8"被ダメ-20%% / 与ダメ-30%%");
				break;
			case Transfer::RoguelikeUpgrade::ArtifactGlassShoes:
				sprintf_s(out, outSize, u8"CT-20%% / CTスキルダメージ-40%%");
				break;
			case Transfer::RoguelikeUpgrade::ArtifactMagicPiggyBank:
				sprintf_s(out, outSize, u8"3戦報酬保留後にまとめ受取 + 追加2報酬");
				break;
			case Transfer::RoguelikeUpgrade::ArtifactDiceBox:
				sprintf_s(out, outSize, u8"休憩所 / ボス突入時にサイコロ+1");
				break;
			default:
				sprintf_s(out, outSize, u8"魔道具を取得");
				break;
			}
			return;
		default:
			out[0] = '\0';
			return;
		}
	}

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

static void DrawCenteredOverlayText(
	const char* text,
	ImU32 fillColor,
	ImU32 borderColor,
	float widthRatio = 0.0f,
	float minHeightRatio = 0.0f,
	float emphasisScale = 1.0f)
{
	if (!text || text[0] == '\0') return;

	ImGuiViewport* vp = ImGui::GetMainViewport();
	if (!vp) return;
	ImDrawList* dl = ImGui::GetForegroundDrawList(vp);
	if (!dl) return;

	ImFont* font = ImGui::GetFont();
	if (!font) return;

	const float viewportScaleRaw = (vp->Size.x < vp->Size.y) ? (vp->Size.x / 1280.0f) : (vp->Size.y / 720.0f);
	const float viewportScale = (viewportScaleRaw < 0.95f) ? 0.95f : ((viewportScaleRaw > 1.65f) ? 1.65f : viewportScaleRaw);
	const float fontSize = ImGui::GetFontSize() * viewportScale * emphasisScale;
	const ImVec2 pad(18.0f * viewportScale, 16.0f * viewportScale);
	const float requestedWidth = (widthRatio > 0.0f) ? (vp->Size.x * widthRatio) : 0.0f;
	const float maxBoxWidth = vp->Size.x * 0.88f;
	float boxWidth = (requestedWidth > 0.0f) ? requestedWidth : maxBoxWidth;
	if (boxWidth > maxBoxWidth) boxWidth = maxBoxWidth;
	const float wrapWidth = boxWidth - pad.x * 2.0f;
	const ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, wrapWidth, text);
	if (requestedWidth <= 0.0f)
	{
		boxWidth = textSize.x + pad.x * 2.0f;
		if (boxWidth > maxBoxWidth) boxWidth = maxBoxWidth;
	}

	float boxHeight = textSize.y + pad.y * 2.0f;
	if (minHeightRatio > 0.0f)
	{
		const float minBoxHeight = vp->Size.y * minHeightRatio;
		if (boxHeight < minBoxHeight) boxHeight = minBoxHeight;
	}

	const ImVec2 boxMin(vp->Pos.x + (vp->Size.x - boxWidth) * 0.5f,
						vp->Pos.y + (vp->Size.y - boxHeight) * 0.5f);
	const ImVec2 boxMax(boxMin.x + boxWidth, boxMin.y + boxHeight);

	dl->AddRectFilled(boxMin, boxMax, fillColor, 10.0f * viewportScale);
	dl->AddRect(boxMin, boxMax, borderColor, 10.0f * viewportScale);
	dl->AddText(font, fontSize, ImVec2(boxMin.x + pad.x, boxMin.y + pad.y), IM_COL32(255, 255, 255, 255), text, nullptr, wrapWidth);
}

static ImU32 GetRunRewardNodeColor(int rewardType)
{
	switch (rewardType)
	{
	case Transfer::RoguelikeUpgrade::RewardSkill:
		return IM_COL32(70, 150, 255, 255);
	case Transfer::RoguelikeUpgrade::RewardTrait:
		return IM_COL32(106, 214, 132, 255);
	case Transfer::RoguelikeUpgrade::RewardTag:
		return IM_COL32(206, 118, 255, 255);
	case Transfer::RoguelikeUpgrade::RewardWeapon:
		return IM_COL32(244, 178, 70, 255);
	case Transfer::RoguelikeUpgrade::RewardDice:
		return IM_COL32(238, 226, 120, 255);
	case Transfer::RoguelikeUpgrade::RewardArtifact:
		return IM_COL32(96, 224, 218, 255);
	case Transfer::RoguelikeUpgrade::RewardShop:
		return IM_COL32(226, 166, 104, 255);
	case Transfer::RoguelikeUpgrade::RewardRest:
		return IM_COL32(112, 198, 174, 255);
	case Transfer::RoguelikeUpgrade::RewardBoss:
		return IM_COL32(238, 92, 82, 255);
	case Transfer::RoguelikeUpgrade::RewardFinalBoss:
		return IM_COL32(214, 42, 72, 255);
	default:
		return IM_COL32(150, 158, 176, 255);
	}
}

static ImU32 ApplyAlpha(ImU32 color, int alpha)
{
	if (alpha < 0) alpha = 0;
	if (alpha > 255) alpha = 255;
	return (color & IM_COL32(255, 255, 255, 0)) | static_cast<ImU32>(alpha << IM_COL32_A_SHIFT);
}

static void FormatRunNodeLabel(
	const Transfer& tran,
	int stageIndex,
	char* out,
	size_t outSize)
{
	if (!out || outSize == 0) return;
	if (stageIndex < 0 || stageIndex >= tran.GetRunStageCount())
	{
		sprintf_s(out, outSize, "-");
		return;
	}

	const int map = tran.GetRunStageMapNumberAt(stageIndex);
	const int step = tran.GetRunStageStepNumberAt(stageIndex);
	const int rewardType = tran.GetRunRewardTypeAt(stageIndex);
	if (step > 0)
	{
		sprintf_s(out, outSize, u8"M%d-%d", map, step);
	}
	else
	{
		sprintf_s(out, outSize, u8"M%d %s", map, GetRunRewardTypeName(rewardType));
	}
}

static void DrawMapNode(
	ImDrawList* dl,
	ImFont* font,
	const ImVec2& center,
	float radius,
	ImU32 color,
	const char* mainLabel,
	const char* subLabel,
	bool selected,
	bool current,
	float textSize)
{
	if (!dl || !font) return;

	const ImU32 fill = selected ? color : ApplyAlpha(color, current ? 235 : 170);
	const ImU32 ring = selected ? IM_COL32(255, 255, 255, 255) : (current ? IM_COL32(245, 245, 225, 235) : IM_COL32(120, 132, 154, 180));
	if (selected)
	{
		const float pulse = 0.5f + 0.5f * static_cast<float>(std::sin(ImGui::GetTime() * 4.0));
		dl->AddCircleFilled(center, radius + 9.0f + pulse * 4.0f, ApplyAlpha(color, 58 + static_cast<int>(pulse * 42.0f)), 36);
		dl->AddCircle(center, radius + 10.0f + pulse * 2.0f, ApplyAlpha(color, 150 + static_cast<int>(pulse * 55.0f)), 36, 2.0f);
	}
	dl->AddCircleFilled(center, radius, fill, 36);
	dl->AddCircle(center, radius, ring, 36, selected ? 3.0f : 2.0f);
	if (current)
	{
		dl->AddCircle(center, radius + 5.0f, IM_COL32(255, 240, 170, 210), 36, 2.0f);
	}

	if (mainLabel && mainLabel[0] != '\0')
	{
		const ImVec2 textSizeVec = font->CalcTextSizeA(textSize, FLT_MAX, 0.0f, mainLabel);
		dl->AddText(font, textSize, ImVec2(center.x - textSizeVec.x * 0.5f, center.y - radius - textSize * 1.75f), IM_COL32(244, 248, 255, 255), mainLabel);
	}
	if (subLabel && subLabel[0] != '\0')
	{
		const ImVec2 textSizeVec = font->CalcTextSizeA(textSize, FLT_MAX, 0.0f, subLabel);
		dl->AddText(font, textSize, ImVec2(center.x - textSizeVec.x * 0.5f, center.y + radius + textSize * 0.55f), IM_COL32(210, 222, 238, 235), subLabel);
	}
}

static void DrawRunMapSelectionOverlay(const Transfer& tran)
{
	ImGuiViewport* vp = ImGui::GetMainViewport();
	if (!vp) return;
	ImDrawList* dl = ImGui::GetForegroundDrawList(vp);
	if (!dl) return;
	ImFont* font = ImGui::GetFont();
	if (!font) return;

	const int nextStageIndex = tran.roguelike.currentStageIndex + 1;
	if (nextStageIndex < 0 || nextStageIndex >= tran.GetRunStageCount()) return;

	int optionCount = tran.GetRunStageOptionCount(nextStageIndex);
	if (optionCount <= 0)
	{
		DrawCenteredOverlayText(u8"次マス候補がありません", IM_COL32(0, 0, 0, 210), IM_COL32(255, 255, 255, 160));
		return;
	}
	if (optionCount > Transfer::RoguelikeUpgrade::kOfferCount) optionCount = Transfer::RoguelikeUpgrade::kOfferCount;
	int optionIndex = tran.gameplayDebug.rewardSelectionIndex;
	if (optionIndex < 0) optionIndex = 0;
	if (optionIndex >= optionCount) optionIndex = optionCount - 1;

	const float viewportScaleRaw = (vp->Size.x < vp->Size.y) ? (vp->Size.x / 1280.0f) : (vp->Size.y / 720.0f);
	const float viewportScale = ClampFloat(viewportScaleRaw, 0.90f, 1.55f);
	const ImVec2 panelSize(vp->Size.x * 0.82f, vp->Size.y * 0.66f);
	const ImVec2 panelMin(vp->Pos.x + (vp->Size.x - panelSize.x) * 0.5f, vp->Pos.y + (vp->Size.y - panelSize.y) * 0.5f);
	const ImVec2 panelMax(panelMin.x + panelSize.x, panelMin.y + panelSize.y);
	const float corner = 14.0f * viewportScale;
	const float pad = 28.0f * viewportScale;
	const float titleSize = ImGui::GetFontSize() * 1.46f * viewportScale;
	const float bodySize = ImGui::GetFontSize() * 1.03f * viewportScale;
	const float smallSize = ImGui::GetFontSize() * 0.88f * viewportScale;

	dl->AddRectFilled(vp->Pos, ImVec2(vp->Pos.x + vp->Size.x, vp->Pos.y + vp->Size.y), IM_COL32(0, 0, 0, 150));
	dl->AddRectFilled(panelMin, panelMax, IM_COL32(14, 20, 30, 236), corner);
	dl->AddRect(panelMin, panelMax, IM_COL32(180, 206, 238, 190), corner, 0, 2.0f);

	char title[128]{};
	const int nextMap = tran.GetRunStageMapNumberAt(nextStageIndex);
	sprintf_s(title, sizeof(title), u8"次マス選択  Map%d", nextMap);
	dl->AddText(font, titleSize, ImVec2(panelMin.x + pad, panelMin.y + 22.0f * viewportScale), IM_COL32(246, 249, 255, 255), title);

	char status[160]{};
	sprintf_s(status, sizeof(status), u8"所持リロール %d    復活残り %d", tran.roguelike.rerollRemain, tran.roguelike.reviveRemain);
	const ImVec2 statusSize = font->CalcTextSizeA(bodySize, FLT_MAX, 0.0f, status);
	dl->AddText(font, bodySize, ImVec2(panelMax.x - pad - statusSize.x, panelMin.y + 30.0f * viewportScale), IM_COL32(206, 220, 236, 235), status);

	const float mapTop = panelMin.y + 108.0f * viewportScale;
	const float mapBottom = panelMax.y - 120.0f * viewportScale;
	const float centerY = (mapTop + mapBottom) * 0.5f;
	const float currentX = panelMin.x + panelSize.x * 0.22f;
	const float optionX = panelMin.x + panelSize.x * 0.54f;
	const float nextX = panelMin.x + panelSize.x * 0.80f;
	const float radius = 32.0f * viewportScale;
	const float optionGap = 92.0f * viewportScale;

	char currentMain[64]{};
	char currentSub[64]{};
	FormatRunNodeLabel(tran, tran.roguelike.currentStageIndex, currentMain, sizeof(currentMain));
	sprintf_s(currentSub, sizeof(currentSub), u8"現在");
	DrawMapNode(dl, font, ImVec2(currentX, centerY), radius, GetRunRewardNodeColor(tran.GetCurrentRunRewardType()), currentMain, currentSub, false, true, bodySize);

	ImVec2 optionCenters[Transfer::RoguelikeUpgrade::kOfferCount]{};
	const float firstOptionY = centerY - optionGap * static_cast<float>(optionCount - 1) * 0.5f;
	for (int i = 0; i < optionCount; ++i)
	{
		optionCenters[i] = ImVec2(optionX, firstOptionY + optionGap * static_cast<float>(i));
		dl->AddBezierCubic(
			ImVec2(currentX + radius + 8.0f * viewportScale, centerY),
			ImVec2(currentX + panelSize.x * 0.10f, centerY),
			ImVec2(optionX - panelSize.x * 0.10f, optionCenters[i].y),
			ImVec2(optionCenters[i].x - radius - 8.0f * viewportScale, optionCenters[i].y),
			(i == optionIndex) ? IM_COL32(240, 248, 255, 235) : IM_COL32(110, 132, 160, 145),
			(i == optionIndex) ? 4.0f : 2.0f);
	}

	for (int i = 0; i < optionCount; ++i)
	{
		const int rewardType = tran.GetRunStageOptionRewardType(nextStageIndex, i);
		char nodeMain[64]{};
		char nodeSub[64]{};
		sprintf_s(nodeMain, sizeof(nodeMain), u8"%s", GetRunRewardTypeName(rewardType));
		const int nextStep = tran.GetRunStageStepNumberAt(nextStageIndex);
		if (nextStep > 0)
		{
			sprintf_s(nodeSub, sizeof(nodeSub), u8"Step%d", nextStep);
		}
		else
		{
			sprintf_s(nodeSub, sizeof(nodeSub), u8"次のマス");
		}
		DrawMapNode(dl, font, optionCenters[i], (i == optionIndex) ? radius * 1.12f : radius, GetRunRewardNodeColor(rewardType), nodeMain, nodeSub, i == optionIndex, false, bodySize);
	}

	const int futureStageIndex = nextStageIndex + 1;
	if (futureStageIndex < tran.GetRunStageCount())
	{
		char futureMain[64]{};
		char futureSub[64]{};
		FormatRunNodeLabel(tran, futureStageIndex, futureMain, sizeof(futureMain));
		sprintf_s(futureSub, sizeof(futureSub), u8"その先");
		for (int i = 0; i < optionCount; ++i)
		{
			dl->AddBezierCubic(
				ImVec2(optionCenters[i].x + radius + 8.0f * viewportScale, optionCenters[i].y),
				ImVec2(optionX + panelSize.x * 0.10f, optionCenters[i].y),
				ImVec2(nextX - panelSize.x * 0.10f, centerY),
				ImVec2(nextX - radius - 8.0f * viewportScale, centerY),
				(i == optionIndex) ? IM_COL32(240, 248, 255, 170) : IM_COL32(92, 110, 132, 105),
				(i == optionIndex) ? 3.0f : 1.5f);
		}
		DrawMapNode(dl, font, ImVec2(nextX, centerY), radius * 0.82f, GetRunRewardNodeColor(tran.GetRunRewardTypeAt(futureStageIndex)), futureMain, futureSub, false, false, smallSize);
	}

	const float stripY = panelMax.y - 72.0f * viewportScale;
	const float stripLeft = panelMin.x + pad;
	const float stripRight = panelMax.x - pad;
	dl->AddLine(ImVec2(stripLeft, stripY), ImVec2(stripRight, stripY), IM_COL32(82, 98, 122, 170), 2.0f);

	int visibleFirst = tran.roguelike.currentStageIndex - 2;
	if (visibleFirst < 0) visibleFirst = 0;
	int visibleLast = visibleFirst + 7;
	if (visibleLast >= tran.GetRunStageCount())
	{
		visibleLast = tran.GetRunStageCount() - 1;
		visibleFirst = visibleLast - 7;
		if (visibleFirst < 0) visibleFirst = 0;
	}
	const int visibleCount = visibleLast - visibleFirst + 1;
	for (int i = 0; i < visibleCount; ++i)
	{
		const int stageIndex = visibleFirst + i;
		const float t = (visibleCount <= 1) ? 0.0f : static_cast<float>(i) / static_cast<float>(visibleCount - 1);
		const float x = stripLeft + (stripRight - stripLeft) * t;
		const bool current = stageIndex == tran.roguelike.currentStageIndex;
		const bool next = stageIndex == nextStageIndex;
		const float miniRadius = (current || next) ? 8.0f * viewportScale : 5.5f * viewportScale;
		const ImU32 color = GetRunRewardNodeColor(stageIndex == nextStageIndex
			? tran.GetRunStageOptionRewardType(nextStageIndex, optionIndex)
			: tran.GetRunRewardTypeAt(stageIndex));
		dl->AddCircleFilled(ImVec2(x, stripY), miniRadius, current ? IM_COL32(255, 245, 180, 255) : (next ? color : ApplyAlpha(color, 180)), 20);
		if (current || next)
		{
			dl->AddCircle(ImVec2(x, stripY), miniRadius + 4.0f * viewportScale, IM_COL32(245, 248, 255, 220), 20, 2.0f);
		}
	}

	const char* help = u8"[方向キー / 左スティック] 選択    [Controller Confirm / Enter / F / Space] 決定";
	const ImVec2 helpSize = font->CalcTextSizeA(smallSize, FLT_MAX, 0.0f, help);
	dl->AddText(font, smallSize, ImVec2(panelMax.x - pad - helpSize.x, panelMax.y - 34.0f * viewportScale), IM_COL32(204, 218, 236, 230), help);
}

static ImU32 GetOfferCardColor(int offerType)
{
	if (offerType < 0)
	{
		return IM_COL32(116, 124, 140, 255);
	}
	if (offerType >= Transfer::RoguelikeUpgrade::kOfferTypeStride)
	{
		switch (GetPackedOfferType(offerType))
		{
		case Transfer::RoguelikeUpgrade::OfferTrait:
			return GetRunRewardNodeColor(Transfer::RoguelikeUpgrade::RewardTrait);
		case Transfer::RoguelikeUpgrade::OfferWeaponUpgrade:
			return GetRunRewardNodeColor(Transfer::RoguelikeUpgrade::RewardWeapon);
		case Transfer::RoguelikeUpgrade::OfferSkillEnhance:
		case Transfer::RoguelikeUpgrade::OfferSkillChange:
			return GetRunRewardNodeColor(Transfer::RoguelikeUpgrade::RewardSkill);
		case Transfer::RoguelikeUpgrade::OfferTagDisable:
			return GetRunRewardNodeColor(Transfer::RoguelikeUpgrade::RewardTag);
		case Transfer::RoguelikeUpgrade::OfferArtifact:
			return GetRunRewardNodeColor(Transfer::RoguelikeUpgrade::RewardArtifact);
		default:
			return IM_COL32(150, 158, 176, 255);
		}
	}
	return IM_COL32(154, 188, 236, 255);
}

static void ResolveRewardSelectionText(const Transfer& tran, char* title, size_t titleSize, char* continueLabel, size_t continueSize)
{
	if (title && titleSize > 0)
	{
		sprintf_s(title, titleSize, "%s", GetRunRewardTypeName(tran.GetCurrentRunRewardType()));
	}
	if (continueLabel && continueSize > 0)
	{
		const bool hasFollowupReward = (tran.roguelike.selectionRoundsRemaining > 1);
		const bool isShopSelection =
			tran.roguelike.intermissionMode == Transfer::RoguelikeUpgrade::IntermissionShop ||
			tran.roguelike.intermissionMode == Transfer::RoguelikeUpgrade::IntermissionRest;
		const char* label = hasFollowupReward ? u8"次の報酬へ" : u8"マップ選択へ";
		if (!hasFollowupReward && tran.gameplayDebug.challengeReturnToGameAfterReward != 0)
		{
			label = u8"ゲームへ戻る";
		}
		if (isShopSelection)
		{
			label = (tran.roguelike.intermissionMode == Transfer::RoguelikeUpgrade::IntermissionRest) ? u8"休憩所へ戻る" : u8"ショップへ戻る";
		}
		sprintf_s(continueLabel, continueSize, "%s", label);
	}

	const int selectionPhase = tran.roguelike.selectionPhase;
	const char* phaseTitle = nullptr;
	switch (selectionPhase)
	{
	case Transfer::RoguelikeUpgrade::SelectionRewardSkill:
	case Transfer::RoguelikeUpgrade::SelectionShopSkillEnhance:
		phaseTitle = u8"スキル強化";
		break;
	case Transfer::RoguelikeUpgrade::SelectionRewardTrait:
	case Transfer::RoguelikeUpgrade::SelectionShopTrait:
		phaseTitle = u8"特性強化";
		break;
	case Transfer::RoguelikeUpgrade::SelectionRewardTag:
		phaseTitle = u8"タグ解除";
		break;
	case Transfer::RoguelikeUpgrade::SelectionRewardWeapon:
		phaseTitle = u8"武器強化";
		break;
	case Transfer::RoguelikeUpgrade::SelectionRewardArtifact:
		phaseTitle = u8"魔道具";
		break;
	case Transfer::RoguelikeUpgrade::SelectionShopSkillChange:
		phaseTitle = u8"スキル変更";
		break;
	case Transfer::RoguelikeUpgrade::SelectionMixed:
		phaseTitle = u8"報酬";
		break;
	default:
		break;
	}
	if (phaseTitle && title && titleSize > 0)
	{
		sprintf_s(title, titleSize, "%s", phaseTitle);
	}
}

static void DrawRewardCard(
	ImDrawList* dl,
	ImFont* font,
	const ImVec2& min,
	const ImVec2& max,
	const char* label,
	const char* description,
	ImU32 accent,
	bool selected,
	bool disabled,
	float scale)
{
	const float corner = 14.0f * scale;
	const ImU32 fill = selected ? IM_COL32(36, 48, 70, 242) : IM_COL32(24, 31, 43, 230);
	const ImU32 border = selected ? IM_COL32(244, 248, 255, 245) : IM_COL32(92, 108, 132, 165);
	const float pulse = selected ? (0.5f + 0.5f * static_cast<float>(std::sin(ImGui::GetTime() * 4.0))) : 0.0f;
	if (selected)
	{
		dl->AddRectFilled(ImVec2(min.x - 5.0f * scale, min.y - 5.0f * scale), ImVec2(max.x + 5.0f * scale, max.y + 5.0f * scale), ApplyAlpha(accent, 38 + static_cast<int>(pulse * 40.0f)), corner + 5.0f * scale);
	}
	dl->AddRectFilled(min, max, disabled ? IM_COL32(18, 22, 30, 205) : fill, corner);
	dl->AddRect(min, max, disabled ? IM_COL32(70, 78, 92, 120) : border, corner, 0, selected ? 2.4f : 1.4f);
	dl->AddRectFilled(min, ImVec2(min.x + 7.0f * scale, max.y), disabled ? IM_COL32(70, 78, 92, 155) : accent, corner, ImDrawFlags_RoundCornersLeft);

	const float titleSize = ImGui::GetFontSize() * 1.10f * scale;
	const float bodySize = ImGui::GetFontSize() * 0.92f * scale;
	const ImU32 titleColor = disabled ? IM_COL32(142, 150, 164, 210) : IM_COL32(248, 250, 255, 255);
	const ImU32 bodyColor = disabled ? IM_COL32(120, 128, 142, 190) : IM_COL32(204, 216, 232, 238);
	dl->AddText(font, titleSize, ImVec2(min.x + 24.0f * scale, min.y + 18.0f * scale), titleColor, label ? label : "-");
	dl->AddText(font, bodySize, ImVec2(min.x + 24.0f * scale, min.y + 56.0f * scale), bodyColor, description ? description : "", nullptr, max.x - min.x - 44.0f * scale);
}

static void DrawRewardSelectionCardOverlay(const Transfer& tran)
{
	ImGuiViewport* vp = ImGui::GetMainViewport();
	if (!vp) return;
	ImDrawList* dl = ImGui::GetForegroundDrawList(vp);
	if (!dl) return;
	ImFont* font = ImGui::GetFont();
	if (!font) return;

	const float scaleRaw = (vp->Size.x < vp->Size.y) ? (vp->Size.x / 1280.0f) : (vp->Size.y / 720.0f);
	const float scale = ClampFloat(scaleRaw, 0.90f, 1.55f);
	const ImVec2 panelSize(vp->Size.x * 0.78f, vp->Size.y * 0.68f);
	const ImVec2 panelMin(vp->Pos.x + (vp->Size.x - panelSize.x) * 0.5f, vp->Pos.y + (vp->Size.y - panelSize.y) * 0.5f);
	const ImVec2 panelMax(panelMin.x + panelSize.x, panelMin.y + panelSize.y);
	const float pad = 30.0f * scale;
	const float corner = 16.0f * scale;

	dl->AddRectFilled(vp->Pos, ImVec2(vp->Pos.x + vp->Size.x, vp->Pos.y + vp->Size.y), IM_COL32(0, 0, 0, 158));
	dl->AddRectFilled(panelMin, panelMax, IM_COL32(15, 19, 28, 238), corner);
	dl->AddRect(panelMin, panelMax, IM_COL32(178, 206, 238, 185), corner, 0, 2.0f);

	char title[96]{};
	char continueLabel[96]{};
	ResolveRewardSelectionText(tran, title, sizeof(title), continueLabel, sizeof(continueLabel));
	char subtitle[192]{};
	sprintf_s(subtitle, sizeof(subtitle), u8"報酬を1つ選択   残り報酬 %d   リロール %d", tran.roguelike.selectionRoundsRemaining, tran.roguelike.rerollRemain);

	const float titleSize = ImGui::GetFontSize() * 1.55f * scale;
	const float bodySize = ImGui::GetFontSize() * 0.96f * scale;
	dl->AddText(font, titleSize, ImVec2(panelMin.x + pad, panelMin.y + 24.0f * scale), IM_COL32(248, 250, 255, 255), title);
	dl->AddText(font, bodySize, ImVec2(panelMin.x + pad, panelMin.y + 70.0f * scale), IM_COL32(198, 214, 234, 235), subtitle);
	dl->AddLine(ImVec2(panelMin.x + pad, panelMin.y + 105.0f * scale), ImVec2(panelMax.x - pad, panelMin.y + 105.0f * scale), IM_COL32(100, 118, 144, 170), 1.5f);

	int validSlots[Transfer::RoguelikeUpgrade::kOfferCount]{};
	int validCount = 0;
	for (int i = 0; i < Transfer::RoguelikeUpgrade::kOfferCount; ++i)
	{
		if (tran.roguelike.offers[i] >= 0)
		{
			validSlots[validCount++] = i;
		}
	}
	int selectedOption = tran.gameplayDebug.rewardSelectionIndex;
	if (selectedOption < 0) selectedOption = 0;
	if (validCount > 0 && selectedOption >= validCount) selectedOption = validCount - 1;
	const int selectedSlot = (validCount > 0) ? validSlots[selectedOption] : 0;

	if (validCount <= 0)
	{
		const ImVec2 cardMin(panelMin.x + pad, panelMin.y + 150.0f * scale);
		const ImVec2 cardMax(panelMax.x - pad, cardMin.y + 120.0f * scale);
		DrawRewardCard(dl, font, cardMin, cardMax, continueLabel, u8"取得できる候補がないため、次の処理へ進みます。", IM_COL32(150, 158, 176, 255), true, false, scale);
	}
	else
	{
		const float cardW = (panelSize.x - pad * 2.0f - 22.0f * scale * 2.0f) / 3.0f;
		const float cardH = 220.0f * scale;
		const float cardY = panelMin.y + 145.0f * scale;
		for (int i = 0; i < Transfer::RoguelikeUpgrade::kOfferCount; ++i)
		{
			char label[96]{};
			char desc[160]{};
			const int offer = tran.roguelike.offers[i];
			FormatUpgradeLabel(tran, offer, label, sizeof(label));
			FormatUpgradeDescription(tran, offer, desc, sizeof(desc));
			const ImVec2 cardMin(panelMin.x + pad + static_cast<float>(i) * (cardW + 22.0f * scale), cardY);
			const ImVec2 cardMax(cardMin.x + cardW, cardY + cardH);
			DrawRewardCard(dl, font, cardMin, cardMax, label, desc, GetOfferCardColor(offer), selectedSlot == i, offer < 0, scale);
		}
	}

	const char* help = u8"[方向キー / 左スティック] 選択    [Controller Confirm / Enter / F / Space] 決定    [R / Controller Reroll] リロール";
	const ImVec2 helpSize = font->CalcTextSizeA(bodySize, FLT_MAX, 0.0f, help);
	dl->AddText(font, bodySize, ImVec2(panelMax.x - pad - helpSize.x, panelMax.y - 38.0f * scale), IM_COL32(202, 216, 234, 232), help);
}

static void DrawRunShopRestOverlay(const Transfer& tran)
{
	ImGuiViewport* vp = ImGui::GetMainViewport();
	if (!vp) return;
	ImDrawList* dl = ImGui::GetForegroundDrawList(vp);
	if (!dl) return;
	ImFont* font = ImGui::GetFont();
	if (!font) return;

	const bool isRest = (tran.roguelike.intermissionMode == Transfer::RoguelikeUpgrade::IntermissionRest);
	const float scaleRaw = (vp->Size.x < vp->Size.y) ? (vp->Size.x / 1280.0f) : (vp->Size.y / 720.0f);
	const float scale = ClampFloat(scaleRaw, 0.90f, 1.55f);
	const ImVec2 panelSize(vp->Size.x * 0.74f, vp->Size.y * 0.66f);
	const ImVec2 panelMin(vp->Pos.x + (vp->Size.x - panelSize.x) * 0.5f, vp->Pos.y + (vp->Size.y - panelSize.y) * 0.5f);
	const ImVec2 panelMax(panelMin.x + panelSize.x, panelMin.y + panelSize.y);
	const float pad = 30.0f * scale;
	const float corner = 16.0f * scale;

	dl->AddRectFilled(vp->Pos, ImVec2(vp->Pos.x + vp->Size.x, vp->Pos.y + vp->Size.y), IM_COL32(0, 0, 0, 154));
	dl->AddRectFilled(panelMin, panelMax, isRest ? IM_COL32(14, 30, 30, 238) : IM_COL32(27, 22, 16, 238), corner);
	dl->AddRect(panelMin, panelMax, isRest ? IM_COL32(142, 224, 202, 190) : IM_COL32(232, 190, 126, 190), corner, 0, 2.0f);

	const float titleSize = ImGui::GetFontSize() * 1.55f * scale;
	const float bodySize = ImGui::GetFontSize() * 0.96f * scale;
	const char* title = isRest ? u8"休憩所" : u8"ショップ";
	dl->AddText(font, titleSize, ImVec2(panelMin.x + pad, panelMin.y + 24.0f * scale), IM_COL32(248, 250, 255, 255), title);
	char status[192]{};
	sprintf_s(status, sizeof(status), u8"進行 %d / %d    所持リロール %d    購入コスト %d", tran.roguelike.currentStageIndex + 1, tran.GetRunStageCount(), tran.roguelike.rerollRemain, tran.GetCurrentShopCost());
	dl->AddText(font, bodySize, ImVec2(panelMin.x + pad, panelMin.y + 70.0f * scale), IM_COL32(210, 222, 238, 236), status);
	if (isRest)
	{
		dl->AddText(font, bodySize, ImVec2(panelMin.x + pad, panelMin.y + 96.0f * scale), IM_COL32(164, 236, 214, 238), u8"最大HPの25%を回復済み。必要なら強化を購入してから次へ進めます。");
	}
	dl->AddLine(ImVec2(panelMin.x + pad, panelMin.y + 126.0f * scale), ImVec2(panelMax.x - pad, panelMin.y + 126.0f * scale), IM_COL32(120, 132, 150, 165), 1.5f);

	int selected = tran.gameplayDebug.rewardSelectionIndex;
	if (selected < 0) selected = 0;
	if (selected > 3) selected = 3;
	const char* labels[4] = { u8"特性強化", u8"スキル変更", u8"スキル強化", u8"次へ進む" };
	const char* descs[4] =
	{
		u8"リロールアイテムを消費して、特性強化候補を開きます。",
		u8"獲得済みスキルを別スキルへ交換します。強化内容は引き継ぎます。",
		u8"現在のスキルに追加強化を付けます。",
		u8"このマスを終了して次の進行へ移ります。"
	};
	const ImU32 colors[4] =
	{
		GetRunRewardNodeColor(Transfer::RoguelikeUpgrade::RewardTrait),
		GetRunRewardNodeColor(Transfer::RoguelikeUpgrade::RewardSkill),
		GetRunRewardNodeColor(Transfer::RoguelikeUpgrade::RewardSkill),
		IM_COL32(190, 198, 214, 255)
	};
	const float rowH = 78.0f * scale;
	const float rowGap = 12.0f * scale;
	float y = panelMin.y + 155.0f * scale;
	for (int i = 0; i < 4; ++i)
	{
		char label[96]{};
		if (i < 3)
		{
			sprintf_s(label, sizeof(label), u8"%s  cost %d", labels[i], tran.GetCurrentShopCost());
		}
		else
		{
			sprintf_s(label, sizeof(label), "%s", labels[i]);
		}
		DrawRewardCard(dl, font, ImVec2(panelMin.x + pad, y), ImVec2(panelMax.x - pad, y + rowH), label, descs[i], colors[i], selected == i, false, scale);
		y += rowH + rowGap;
	}

	const char* help = u8"[方向キー / 左スティック] 選択    [Controller Confirm / Enter / F / Space] 決定";
	const ImVec2 helpSize = font->CalcTextSizeA(bodySize, FLT_MAX, 0.0f, help);
	dl->AddText(font, bodySize, ImVec2(panelMax.x - pad - helpSize.x, panelMax.y - 34.0f * scale), IM_COL32(216, 226, 240, 232), help);
}
static void FormatUpgradeTableValue(float value, const char* suffix, char* out, size_t outSize)
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

static void FormatUpgradePercentValue(float scale, char* out, size_t outSize)
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

static void SetupUpgradeMatrixColumns()
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
static void DrawUpgradeMatrixRow(const char* name, const char* kind, Formatter formatter)
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
	(void)showBossDebugHint;
	if (SceneManager::GetCurrent() != SceneManager::SceneType::SCENE_RESULT) return;
	if (SceneManager::GetResultType() != SceneManager::ResultType::Win) return;
	if (tran.roguelike.selectionPending != 0)
	{
		DrawRewardSelectionCardOverlay(tran);
		return;
	}

	if (tran.roguelike.intermissionMode == Transfer::RoguelikeUpgrade::IntermissionMapSelect)
	{
		DrawRunMapSelectionOverlay(tran);
		return;
	}

	if (tran.roguelike.intermissionMode == Transfer::RoguelikeUpgrade::IntermissionShop ||
		tran.roguelike.intermissionMode == Transfer::RoguelikeUpgrade::IntermissionRest)
	{
		DrawRunShopRestOverlay(tran);
	}
}

bool IsEngineEditorScene(SceneManager::SceneType sceneType)
{
	return sceneType == SceneManager::SceneType::SCENE_ENGINE_EDITOR;
}

static SceneCastleEditor* GetCastleEditorScene()
{
	if (!IsEngineEditorScene(SceneManager::GetCurrent())) return nullptr;
	return dynamic_cast<SceneCastleEditor*>(SceneManager::GetScene());
}

static RenderTarget* g_pEngineEditorRT = nullptr;
static DepthStencil* g_pEngineEditorDS = nullptr;
static UINT g_engineEditorRTWidth = 0;
static UINT g_engineEditorRTHeight = 0;
static UINT g_engineEditorRequestWidth = 640;
static UINT g_engineEditorRequestHeight = 360;
static bool g_showCastleEditorModelViewWindow = false;

void ReleaseEngineEditorRenderTargets()
{
	SAFE_DELETE(g_pEngineEditorDS);
	SAFE_DELETE(g_pEngineEditorRT);
	g_engineEditorRTWidth = 0;
	g_engineEditorRTHeight = 0;
}

static bool EnsureEngineEditorRenderTarget(UINT width, UINT height)
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

static void DrawCastleEditorToolControls(SceneCastleEditor* editor)
{
	if (!editor) return;

	const int selectedAssetIndex = editor->GetSelectedAssetIndex();
	const SceneCastleEditor::ToolMode toolMode = editor->GetToolMode();

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
}

static void DrawCastleEditorOperationGuide(SceneCastleEditor* editor)
{
	if (!editor) return;

	const int selectedAssetIndex = editor->GetSelectedAssetIndex();
	const SceneCastleEditor::ToolMode toolMode = editor->GetToolMode();

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
		ImGui::TextDisabled(u8"現在: Paint (%s)", asset ? asset->name.c_str() : u8"Unknown");
	}
	else if (toolMode == SceneCastleEditor::ToolMode::PlaceFill)
	{
		const SceneCastleEditor::AssetInfo* asset = editor->GetAssetInfo(selectedAssetIndex);
		ImGui::TextDisabled(u8"現在: Fill (%s)", asset ? asset->name.c_str() : u8"Unknown");
	}
	else
	{
		const SceneCastleEditor::AssetInfo* asset = editor->GetAssetInfo(selectedAssetIndex);
		ImGui::TextDisabled(u8"現在: Place (%s)", asset ? asset->name.c_str() : u8"Unknown");
	}
}

static void DrawCastleEditorModelViewWindow(SceneCastleEditor* editor)
{
	if (!editor || !g_showCastleEditorModelViewWindow) return;

	ImGui::SetNextWindowSize(ImVec2(420.0f, 520.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(u8"ModelView", &g_showCastleEditorModelViewWindow))
	{
		ImGui::End();
		return;
	}

	const int selectedIndex =
		(editor->GetSelectedPlacementCount() == 1) ? editor->GetSelectedPlacementIndex() : -1;
	const int modelViewAssetIndex =
		(editor->GetSelectedAssetIndex() >= 0)
		? editor->GetSelectedAssetIndex()
		: ((selectedIndex >= 0 && editor->GetPlacement(selectedIndex))
			? editor->GetPlacement(selectedIndex)->assetIndex
			: -1);

	if (modelViewAssetIndex >= 0)
	{
		const SceneCastleEditor::AssetInfo* asset = editor->GetAssetInfo(modelViewAssetIndex);
		if (asset)
		{
			ImGui::Text(u8"表示中: %s", asset->name.c_str());
		}

		const float viewSize = (ImGui::GetContentRegionAvail().x < 180.0f)
			? 180.0f
			: ImGui::GetContentRegionAvail().x;
		const ImVec2 imagePos = ImGui::GetCursorScreenPos();
		void* textureId = editor->GetModelViewTextureId(static_cast<unsigned int>(viewSize));
		if (textureId)
		{
			ImGui::Image(textureId, ImVec2(viewSize, viewSize));
		}
		else
		{
			ImGui::InvisibleButton("##model_view_dummy_window", ImVec2(viewSize, viewSize));
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

static void DrawCastleEditorPaletteWindow(SceneCastleEditor* editor)
{
	if (!editor) return;
	static char addAssetName[128] = "";
	static char addAssetPath[512] = "";
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
					asset->name.c_str(),
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

	ImGui::SeparatorText(u8"モデル管理");
	if (ImGui::Button(u8"モデル追加"))
	{
		addAssetName[0] = '\0';
		addAssetPath[0] = '\0';
		ImGui::OpenPopup(u8"モデル追加");
	}
	ImGui::SameLine();
	if (ImGui::Button(u8"再読込"))
	{
		editor->ReloadAssetCatalog();
	}
	ImGui::SameLine();
	const bool canRemoveSelectedAsset = editor->CanRemoveAsset(selectedAssetIndex);
	if (selectedAssetIndex < 0 || !canRemoveSelectedAsset) ImGui::BeginDisabled();
	if (ImGui::Button(u8"選択モデル削除") && selectedAssetIndex >= 0)
	{
		editor->RemoveAsset(selectedAssetIndex);
	}
	if (selectedAssetIndex < 0 || !canRemoveSelectedAsset) ImGui::EndDisabled();

	if (ImGui::BeginPopupModal(u8"モデル追加", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextDisabled(u8"表示名が空欄ならファイル名から自動で作成します。");
		ImGui::TextDisabled(u8"例: Assets/Model/Castle/Brick1.fbx / Assets/Model/Furina/furina.pmx");
		ImGui::PushItemWidth(420.0f);
		ImGui::InputText(u8"表示名", addAssetName, IM_ARRAYSIZE(addAssetName));
		ImGui::InputText(u8"ファイルパス", addAssetPath, IM_ARRAYSIZE(addAssetPath));
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::Button(u8"参照...", ImVec2(90.0f, 0.0f)))
		{
			BrowseCastleEditorModelPath(addAssetPath, IM_ARRAYSIZE(addAssetPath));
		}

		if (ImGui::Button(u8"追加", ImVec2(120.0f, 0.0f)))
		{
			if (editor->AddAssetFromPath(addAssetName, addAssetPath))
			{
				addAssetName[0] = '\0';
				addAssetPath[0] = '\0';
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button(u8"キャンセル", ImVec2(120.0f, 0.0f)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	const char* assetCatalogStatus = editor->GetAssetCatalogStatusMessage();
	if (assetCatalogStatus && assetCatalogStatus[0] != '\0')
	{
		const ImVec4 color = editor->IsAssetCatalogStatusError()
			? ImVec4(0.92f, 0.38f, 0.38f, 1.0f)
			: ImVec4(0.45f, 0.82f, 0.55f, 1.0f);
		ImGui::TextColored(color, "%s", assetCatalogStatus);
	}
	if (selectedAssetIndex >= 0)
	{
		const SceneCastleEditor::AssetInfo* selectedAsset = editor->GetAssetInfo(selectedAssetIndex);
		if (selectedAsset)
		{
			ImGui::TextDisabled(u8"選択中モデル: %s", selectedAsset->name.c_str());
			ImGui::TextDisabled(u8"パス: %s", selectedAsset->path.c_str());
			if (!canRemoveSelectedAsset)
			{
				ImGui::TextDisabled(u8"配置中または履歴参照中のモデルは削除できません。");
			}
		}
	}
	ImGui::TextDisabled(u8"設定ファイル: %s", editor->GetAssetCatalogPath());

	if (ImGui::Button(u8"回転 -90"))
	{
		editor->RotatePreview(-1);
	}
	ImGui::SameLine();
	if (ImGui::Button(u8"回転 +90"))
	{
		editor->RotatePreview(1);
	}

	ImGui::End();
}

static void DrawCastleEditorHierarchyWindow(SceneCastleEditor* editor)
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
			asset ? asset->name.c_str() : "Unknown",
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

static void DrawCastleEditorInspectorWindow(SceneCastleEditor* editor)
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
			ImGui::Text(u8"選択中: %s", asset ? asset->name.c_str() : u8"Unknown");

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

	ImGui::SeparatorText(u8"ModelView");
	const bool canOpenModelView =
		(editor->GetSelectedAssetIndex() >= 0) ||
		(selectedIndex >= 0 && editor->GetPlacement(selectedIndex));
	const bool canToggleModelView = g_showCastleEditorModelViewWindow || canOpenModelView;
	if (!canToggleModelView) ImGui::BeginDisabled();
	if (ImGui::Button(g_showCastleEditorModelViewWindow ? u8"ModelViewを閉じる" : u8"ModelViewを開く"))
	{
		g_showCastleEditorModelViewWindow = !g_showCastleEditorModelViewWindow;
	}
	if (!canToggleModelView) ImGui::EndDisabled();
	if (canOpenModelView)
	{
		ImGui::TextDisabled(
			g_showCastleEditorModelViewWindow
				? u8"専用ウィンドウで表示中です。"
				: u8"ボタンを押すと専用ウィンドウで表示します。");
	}
	else
	{
		ImGui::TextDisabled(u8"アセットか配置オブジェクトを選択すると開けます。");
	}

	ImGui::End();
}

static void DrawCastleEditorCameraWindow(SceneCastleEditor* editor)
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
	DrawCastleEditorToolControls(editor);
	DrawCastleEditorOperationGuide(editor);

	ImGui::End();
}

static void DrawEngineEditorSceneViewWindow(SceneCastleEditor* editor)
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
	DrawCastleEditorModelViewWindow(editor);
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

