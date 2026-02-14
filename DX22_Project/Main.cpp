#include "Main.h"
#include <memory>
#include "DirectX.h"
#include "Geometory.h"
#include "Sprite.h"
#include "Input.h"
#include "SceneManager.h"
#include "Defines.h"
#include "ShaderList.h"
#include "Transfer.h"
#include "Sound.h"
// rand初期化用
#include <cstdlib>
#include <ctime>
#include <cstdio> // 追加

// ImGui
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "Easing.h"

// デバッグ用
#include "DebugUtil.h"

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
		case SceneManager::SceneType::SCENE_3DEDITOR:
			sceneTxt = u8"3Dエディタ";
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
				DragFloat(u8"ダッシュ距離", &tran.player.dashDistance, 0.05f, 0.0f, 10.0f);
				DragFloat(u8"ダッシュCT", &tran.player.dashCooldown, 0.01f, 0.0f, 5.0f);
				DragFloat(u8"ダッシュ時間", &tran.player.dashDuration, 0.01f, 0.01f, 1.0f);
				DragFloat(u8"ステージサイズ", &tran.player.stageSize, 0.1f, 1.0f, 20.0f);
				ColorEdit4(u8"色", reinterpret_cast<float*>(&tran.player.color));
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
					switch (preset)
					{
					case 0: // Easy
						tran.gameplay.enemyCount = 2;
						tran.gameplay.waveMax = 2;
						tran.gameplay.waveEnemyAddPerWave = 1;
						tran.gameplay.enemyAttackWindup = 0.65f;
						tran.gameplay.enemyAttackCooldown = 1.20f;
						tran.gameplay.enemyAttackRangeMin = 0.70f;
						tran.gameplay.enemyAttackRangeScale = 1.20f;
						tran.gameplay.enemyAttackDamage = 0.8f;
						tran.gameplay.enemyMoveSpeed = 1.00f;
						tran.gameplay.waveEnemyMoveSpeedAdd = 0.08f;
						tran.gameplay.waveEnemyAttackDamageScalePerWave = 0.10f;
						break;
					case 2: // Hard
						tran.gameplay.enemyCount = 4;
						tran.gameplay.waveMax = 4;
						tran.gameplay.waveEnemyAddPerWave = 2;
						tran.gameplay.enemyAttackWindup = 0.45f;
						tran.gameplay.enemyAttackCooldown = 0.80f;
						tran.gameplay.enemyAttackRangeMin = 0.90f;
						tran.gameplay.enemyAttackRangeScale = 1.45f;
						tran.gameplay.enemyAttackDamage = 1.3f;
						tran.gameplay.enemyMoveSpeed = 1.35f;
						tran.gameplay.waveEnemyMoveSpeedAdd = 0.22f;
						tran.gameplay.waveEnemyAttackDamageScalePerWave = 0.32f;
						break;
					default: // Normal
						tran.gameplay.enemyCount = 3;
						tran.gameplay.waveMax = 3;
						tran.gameplay.waveEnemyAddPerWave = 1;
						tran.gameplay.enemyAttackWindup = 0.55f;
						tran.gameplay.enemyAttackCooldown = 1.00f;
						tran.gameplay.enemyAttackRangeMin = 0.8f;
						tran.gameplay.enemyAttackRangeScale = 1.35f;
						tran.gameplay.enemyAttackDamage = 1.0f;
						tran.gameplay.enemyMoveSpeed = 1.2f;
						tran.gameplay.waveEnemyMoveSpeedAdd = 0.15f;
						tran.gameplay.waveEnemyAttackDamageScalePerWave = 0.20f;
						break;
					}
					tran.gameplayDebug.difficultyPreset = preset;
				};
				ImGui::SeparatorText(u8"難易度プリセット");
				ImGui::Text(u8"現在: %s", difficultyText);
				if (Button(u8"Easy")) applyDifficultyPreset(0);
				SameLine();
				if (Button(u8"Normal")) applyDifficultyPreset(1);
				SameLine();
				if (Button(u8"Hard")) applyDifficultyPreset(2);

				DragInt(u8"敵数(基準)", &tran.gameplay.enemyCount, 1.0f, 0, 16);
				DragInt(u8"最大Wave", &tran.gameplay.waveMax, 1.0f, 1, 32);
				DragInt(u8"Wave毎の敵追加数", &tran.gameplay.waveEnemyAddPerWave, 1.0f, 0, 16);
				ImGui::TextDisabled(u8"初期値: 敵数3 / 最大Wave3 / Wave追加1");

				ImGui::SeparatorText(u8"プレイヤー攻撃");
				ImGui::TextDisabled(u8"初期値: 準備0.04 / 有効0.12 / 後隙0.10 / CT0.24");
				DragFloat(u8"攻撃準備", &tran.gameplay.attackWindup, 0.005f, 0.0f, 1.0f);
				DragFloat(u8"攻撃有効", &tran.gameplay.attackDuration, 0.005f, 0.01f, 1.0f);
				DragFloat(u8"攻撃後隙", &tran.gameplay.attackRecovery, 0.005f, 0.0f, 1.0f);
				DragFloat(u8"攻撃CT", &tran.gameplay.attackCooldown, 0.005f, 0.0f, 2.0f);
				DragFloat(u8"薙ぎ角度", &tran.gameplay.attackSweepDegrees, 1.0f, 10.0f, 240.0f);
				DragFloat(u8"攻撃半径倍率", &tran.gameplay.attackSweepRadiusScale, 0.01f, 0.1f, 3.0f);
				DragFloat(u8"攻撃幅倍率", &tran.gameplay.attackWidthScale, 0.01f, 0.1f, 3.0f);
				DragFloat(u8"攻撃奥行倍率", &tran.gameplay.attackDepthScale, 0.01f, 0.1f, 3.0f);
				DragFloat(u8"ヒットストップ", &tran.gameplay.attackHitStop, 0.001f, 0.0f, 0.20f);
				DragFloat(u8"ノックバック", &tran.gameplay.attackKnockback, 0.01f, 0.0f, 3.0f);
				DragFloat(u8"ヒット発光", &tran.gameplay.attackHitFlash, 0.005f, 0.0f, 0.50f);
				ImGui::TextDisabled(u8"初期値: 軌跡間隔0.02 / 軌跡残存0.16 / 軌跡倍率0.75");
				DragFloat(u8"攻撃軌跡間隔", &tran.gameplay.attackTrailInterval, 0.002f, 0.0f, 0.20f);
				DragFloat(u8"攻撃軌跡残存", &tran.gameplay.attackTrailLife, 0.005f, 0.0f, 0.50f);
				DragFloat(u8"攻撃軌跡倍率", &tran.gameplay.attackTrailScale, 0.01f, 0.1f, 3.0f);

				ImGui::SeparatorText(u8"被弾・撃破演出");
				ImGui::TextDisabled(u8"初期値: 被弾0.20 / 被弾倍率1.65 / 撃破0.28 / 撃破倍率1.60");
				DragFloat(u8"被弾フラッシュ時間", &tran.gameplay.playerDamageFlash, 0.005f, 0.0f, 1.0f);
				DragFloat(u8"被弾フラッシュ倍率", &tran.gameplay.playerDamageFlashScale, 0.01f, 0.1f, 4.0f);
				DragFloat(u8"敵撃破フラッシュ時間", &tran.gameplay.enemyDefeatFlash, 0.005f, 0.0f, 1.0f);
				DragFloat(u8"敵撃破フラッシュ倍率", &tran.gameplay.enemyDefeatFlashScale, 0.01f, 0.1f, 4.0f);

				ImGui::SeparatorText(u8"音量");
				ImGui::TextDisabled(u8"初期値: Master1.00 / BGM0.70 / SE1.00");
				DragFloat(u8"Master音量", &tran.gameplay.volumeMaster, 0.01f, 0.0f, 2.0f);
				DragFloat(u8"BGM音量", &tran.gameplay.volumeBgm, 0.01f, 0.0f, 2.0f);
				DragFloat(u8"SE音量", &tran.gameplay.volumeSe, 0.01f, 0.0f, 2.0f);

				ImGui::SeparatorText(u8"敵攻撃");
				ImGui::TextDisabled(u8"初期値: 予兆0.55 / CT1.00 / 射程最小0.80 / 射程倍率1.35 / ダメージ1.0");
				DragFloat(u8"敵予兆時間", &tran.gameplay.enemyAttackWindup, 0.01f, 0.0f, 3.0f);
				DragFloat(u8"敵攻撃CT", &tran.gameplay.enemyAttackCooldown, 0.01f, 0.0f, 3.0f);
				DragFloat(u8"敵射程最小", &tran.gameplay.enemyAttackRangeMin, 0.01f, 0.1f, 5.0f);
				DragFloat(u8"敵射程倍率", &tran.gameplay.enemyAttackRangeScale, 0.01f, 0.1f, 5.0f);
				DragFloat(u8"敵ダメージ", &tran.gameplay.enemyAttackDamage, 0.1f, 0.0f, 20.0f);
				DragFloat(u8"敵移動速度", &tran.gameplay.enemyMoveSpeed, 0.01f, 0.1f, 8.0f);
				DragFloat(u8"Wave毎 速度加算", &tran.gameplay.waveEnemyMoveSpeedAdd, 0.01f, 0.0f, 2.0f);
				DragFloat(u8"Wave毎 ダメ倍率加算", &tran.gameplay.waveEnemyAttackDamageScalePerWave, 0.01f, 0.0f, 3.0f);

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
				ImGui::TextDisabled(u8"ゲーム進行: 敵全滅で次Wave、最終Wave全滅で勝利");
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
					sceneTxt = u8"3Dエディタ";
					changeScene = SceneManager::SceneType::SCENE_3DEDITOR;
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
			if (BeginTabItem(u8"UI"))
			{
				tran.diceui.role.pos;
				DragFloat2(u8"ロール 位置", reinterpret_cast<float*>(&tran.diceui.role.pos));
				DragFloat2(u8"ロール サイズ", reinterpret_cast<float*>(&tran.diceui.role.size));
				ColorEdit4(u8"ロール 色", reinterpret_cast<float*>(&tran.diceui.role.color));

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

		ImGui::Text(u8"F1:表  F2:オーバーレイ  F3:デモ");
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

			// Transfer の中身を例として監視
			row_f3("player.pos", tran.player.pos);
			row_f3("player.velocity", tran.player.velocity);
			row_f1("player.hp", tran.player.hp);
			row_f1("player.maxHp", tran.player.maxHp);

			row_i1("enemy.exists", tran.enemy.exists);
			row_f3("enemy.pos", tran.enemy.pos);
			row_f1("enemy.hp", tran.enemy.hp);
			row_f1("enemy.maxHp", tran.enemy.maxHp);
			const char* enemyStateText = u8"なし";
			switch (tran.enemy.state)
			{
			case 0: enemyStateText = u8"徘徊"; break;
			case 1: enemyStateText = u8"追跡"; break;
			default: enemyStateText = u8"なし"; break;
			}
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("enemy.state");
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
			ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("enemy.type");
			ImGui::TableSetColumnIndex(1); ImGui::Text("%d (%s)", tran.enemy.type, enemyTypeText);


			row_f3("dice.pos", tran.dice.pos);
			row_f3("dice.velocity", tran.dice.velocity);
			row_f4("dice.rot", tran.dice.rot);
			row_f1("dice.underVel", tran.dice.underVel);

			row_i1("cameraMode", tran.cameraMode);
			row_i1("gameplay.waveCurrent", tran.gameplayDebug.currentWave);
			row_i1("gameplay.waveMax", tran.gameplayDebug.maxWave);
			row_i1("gameplay.enemiesAlive", tran.gameplayDebug.enemiesAlive);
			row_i1("gameplay.enemiesTarget", tran.gameplayDebug.enemiesTarget);
			row_i1("gameplay.difficultyPreset", tran.gameplayDebug.difficultyPreset);

			ImGui::EndTable();
		}

		ImGui::Spacing();

		// 2) Dice face number のテーブル（小テーブル）
		if (ImGui::CollapsingHeader(u8"サイコロ面（表）", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGuiTableFlags f2 =
				ImGuiTableFlags_Borders |
				ImGuiTableFlags_RowBg |
				ImGuiTableFlags_SizingFixedFit;

			if (ImGui::BeginTable("##faces", MAX_DICE, f2))
			{
				for (int c = 0; c < MAX_DICE; ++c)
				{
					char buf[32];
					sprintf_s(buf, "D%d", c);
					ImGui::TableSetupColumn(buf);
				}
				ImGui::TableHeadersRow();

				ImGui::TableNextRow();
				for (int c = 0; c < MAX_DICE; ++c)
				{
					ImGui::TableSetColumnIndex(c);
					ImGui::Text("%d", tran.dice.currentFaceNumber[c]);
				}

				ImGui::EndTable();
			}
		}

		ImGui::End();
	}

	// ゲーム中は常時HUDを表示（Wave / 残り敵数 / クリア条件）
	if (SceneManager::GetCurrent() == SceneManager::SceneType::SCENE_GAME)
	{
		ImGuiViewport* vp = ImGui::GetMainViewport();
		ImDrawList* dl = ImGui::GetForegroundDrawList(vp);
		const char* difficultyText = u8"Normal";
		switch (tran.gameplayDebug.difficultyPreset)
		{
		case 0: difficultyText = u8"Easy"; break;
		case 2: difficultyText = u8"Hard"; break;
		default: difficultyText = u8"Normal"; break;
		}

		char gameplayHud[256];
		sprintf_s(
			gameplayHud,
			u8"難易度: %s\n現在Wave: %d / %d\n残り敵数: %d / %d\nクリア条件: 最終Waveで敵を全滅",
			difficultyText,
			tran.gameplayDebug.currentWave,
			tran.gameplayDebug.maxWave,
			tran.gameplayDebug.enemiesAlive,
			tran.gameplayDebug.enemiesTarget);

		const ImVec2 pad(10.0f, 8.0f);
		const ImVec2 textSize = ImGui::CalcTextSize(gameplayHud);
		const ImVec2 boxMin(vp->Pos.x + 12.0f, vp->Pos.y + 56.0f);
		const ImVec2 boxMax(boxMin.x + textSize.x + pad.x * 2.0f, boxMin.y + textSize.y + pad.y * 2.0f);

		dl->AddRectFilled(boxMin, boxMax, IM_COL32(0, 0, 0, 170), 6.0f);
		dl->AddRect(boxMin, boxMax, IM_COL32(255, 255, 255, 120), 6.0f);
		dl->AddText(ImVec2(boxMin.x + pad.x, boxMin.y + pad.y), IM_COL32(255, 255, 255, 255), gameplayHud);
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
#endif

	SceneManager::Draw();
	EndDrawDirectX();
}

// EOF

