#pragma once

#include <DirectXMath.h>
#include "Defines.h"

/** @brief Transfer シングルトン参照をローカル変数 `tran` として取り出す簡易マクロです。 */
#define TRAN_INS Transfer &tran = Transfer::GetInstance();
/** @brief `tran` を式として使いたい箇所向けのシングルトン参照取得マクロです。 */
#define TRAN_INS_Get Transfer &tran = Transfer::GetInstance();tran

/**
 * @brief シーン間・システム間で共有するゲーム状態と調整値を保持するシングルトンです。
 */
class Transfer
{
private:
	Transfer() = default;
	~Transfer() = default;

	/**
	 * @brief カメラの Eye / Look をまとめて扱う共有データです。
	 */
	struct CameraInfo
	{
		/** @brief カメラ位置です。 */
		DirectX::XMFLOAT3 eye;
		/** @brief 注視点です。 */
		DirectX::XMFLOAT3 look;
	};
	struct ObjectfromAtoB
	{
		DirectX::XMFLOAT3 A;
		DirectX::XMFLOAT3 Avel;
		DirectX::XMFLOAT3 AangVel;
		DirectX::XMFLOAT3 B;
		DirectX::XMFLOAT3 Bvel;
		DirectX::XMFLOAT3 BangVel;
	};
	struct PlayerInfo
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 size;
		DirectX::XMFLOAT3 velocity;
		float hp = 0.0f;
		float maxHp = 0.0f;
		float moveSpeed = 0.0f;
		float dashDistance = 0.0f;
		float dashCooldown = 0.0f;
		float dashDuration = 0.0f;
		float stageSize = 0.0f;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 lcokColor;
	};
	struct EnemyInfo
	{
		DirectX::XMFLOAT3 pos{ 0.0f, 0.0f, 0.0f };
		float hp = 0.0f;
		float maxHp = 0.0f;
		int state = -1;
		int type = -1;
		int exists = 0;
	};

	struct DiceInfo
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 size;
		DirectX::XMFLOAT3 velocity;
		DirectX::XMFLOAT4 rot;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 lcokColor;
		const float ground = 0.0f;
		// ここからtest用		   
		DirectX::XMFLOAT4X4 world;	// ワールド座標系
		DirectX::XMFLOAT4X4 obj;	// オブジェクト座標系
		DirectX::XMFLOAT3 virtualVelocity;	//仮想運動量
		int currentFaceNumber[MAX_DICE];	// 現在の表面ナンバー
		float underVel = 0.0f; // これ以下の運動量なら停止用変数
	};
	struct UIobj
	{
		DirectX::XMFLOAT2 pos;
		DirectX::XMFLOAT2 size;
		DirectX::XMFLOAT4 color = { 1,1,1,1 };
	};
	struct UIInfo
	{
		UIobj role;
	};
	struct ModelInfo
	{
		DirectX::XMFLOAT3 subAngle;
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 size;
		DirectX::XMFLOAT3 rotate;
	};
	struct ModelBodyInfo
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 size;
		DirectX::XMFLOAT3 angle;
		DirectX::XMFLOAT3 jointRightArmPos;
		DirectX::XMFLOAT3 jointLeftArmPos;
		DirectX::XMFLOAT3 jointRightLegPos;
		DirectX::XMFLOAT3 jointLeftLegPos;
	};
	struct ModelEditer
	{
		ModelInfo armRight1;
		ModelInfo armRight2;
		ModelInfo armLeft1;
		ModelInfo armLeft2;
		ModelInfo legRight1;
		ModelInfo legRight2;
		ModelInfo legLeft1;
		ModelInfo legLeft2;
		ModelBodyInfo body;
	};
	struct Tyabudai
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 size;
		DirectX::XMFLOAT3 rotate;
	};
	struct Arrow
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 rotate;
		DirectX::XMFLOAT3 scale;
	};
	/**
	 * @brief 実行中に調整可能なゲームプレイ設定値です。
	 */
	struct GameplayTuning
	{
		// Stage / wave progression.
		int enemyCount = 3;
		int waveMax = 3;
		int waveEnemyAddPerWave = 1;
		int upgradeStepEasySmall = 1;
		int upgradeStepEasyLarge = 2;
		int upgradeStepNormalSmall = 2;
		int upgradeStepNormalLarge = 3;
		int upgradeStepHardSmall = 3;
		int upgradeStepHardLarge = 5;
		float groundTileSize = 1.0f;
		float cameraIntroDuration = 1.20f;
		float cameraIntroFocusDistance = 2.80f;

		// Player attack / combat feedback.
		float attackWindup = 0.04f;
		float attackDuration = 0.12f;
		float attackRecovery = 0.10f;
		float attackCooldown = 0.24f;
		float skill1Cooldown = 4.0f;
		float skill2Cooldown = 9.0f;
		int screenShakeHitThreshold = 3;
		float screenShakeDuration = 0.18f;
		float screenShakeAmplitude = 0.20f;
		float attackSweepDegrees = 120.0f;
		float attackSweepRadiusScale = 1.25f;
		float attackWidthScale = 1.2f;
		float attackDepthScale = 1.0f;
		float attackHitStop = 0.05f;
		float attackKnockback = 0.45f;
		float attackHitFlash = 0.10f;
		float attackTrailInterval = 0.02f;
		float attackTrailLife = 0.16f;
		float attackTrailScale = 0.75f;
		float playerDamageFlash = 0.20f;
		float playerDamageInvincible = 0.35f;
		float playerDamageFlashScale = 1.65f;
		float enemyDefeatFlash = 0.28f;
		float enemyDefeatFlashScale = 1.60f;

		// Audio / player-facing presentation.
		float volumeMaster = 1.0f;
		float volumeBgm = 0.7f;
		float volumeSe = 1.0f;
		float directionMarkerAlpha = 0.92f;
		float directionMarkerOverlapAlpha = 0.45f;

		// Boss UI / base stats.
		float bossHpBarWidthRate = 0.42f;
		float bossHpBarHeightRate = 0.045f;
		float bossGuardBarOffsetX = 0.0f;
		float bossGuardBarOffsetY = 6.0f;
		float bossGuardBarWidthRate = 0.42f;
		float bossGuardBarHeightRate = 0.018f;
		float bossSizeAreaScale = 6.0f;
		int bossMaxHp = 180;
		float bossAttackTelegraph = 1.0f;
		float bossAttackJumpOutTime = 0.5f;
		float bossAttackDashDuration = 0.35f;
		float bossAttackCooldown = 1.15f;
		float bossAttackLanePlayerScale = 3.0f;
		float bossAttackDamage = 20.0f;
		float bossAttackHitStop = 0.08f;
		float bossHitShakeDuration = 0.18f;
		float bossHitShakeAmplitude = 0.23f;
		float bossGuardInitialMax = 14.0f;
		float bossGuardFinalMax = 24.0f;
		float bossGuardRecoverStep = 2.0f;
		float bossDamageScaleNormal = 0.20f;
		float bossDamageScaleBroken = 2.20f;
		float bossBreakRecoverSec = 8.0f;

		// Boss attack tuning.
		float bossDashNarrowTelegraph = 1.0f;
		float bossDashWideTelegraph = 2.0f;
		float bossDashWideWidthRate = 0.50f;
		int bossRandomRainCount = 5;
		float bossRandomRainTelegraph = 1.0f;
		float bossRandomRainRadiusScale = 1.6f;
		int bossSummonMin = 5;
		int bossSummonMax = 10;
		float bossSummonTelegraph = 1.0f;
		int bossTrackingDropCount = 5;
		float bossTrackingDropTelegraph = 1.0f;
		float bossTrackingDropRadiusScale = 3.0f;
		float bossUltimateCrossTelegraph = 1.0f;
		float bossUltimateCrossLaneScale = 1.0f;
		int bossUltimateStompCount = 5;
		float bossUltimateStompTelegraph = 3.0f;
		float bossUltimateStompRepeatTelegraph = 3.0f;
		float bossUltimateStompRadiusScale = 3.0f;
		float bossUltimateFieldTelegraph = 7.0f;
		float bossUltimateFieldSafeScale = 2.0f;

		// Enemy behavior / projectile tuning.
		float enemyAttackWindup = 0.55f;
		float enemyAttackCooldown = 1.00f;
		float enemyAttackRangeMin = 0.8f;
		float enemyAttackRangeScale = 1.35f;
		float enemyAttackDamage = 1.0f;
		float enemyMoveSpeed = 1.2f;
		float waveEnemyMoveSpeedAdd = 0.15f;
		float waveEnemyAttackDamageScalePerWave = 0.20f;
		float enemyProjectileSpeed = 6.5f;
		float enemyProjectileLife = 1.4f;
		float enemyProjectileRadius = 0.22f;
		float enemyProjectileDamageScale = 0.85f;
		float enemySeparationRadius = 1.1f;
		float enemySeparationWeight = 0.8f;
		float enemySeparationMaxOffset = 0.8f;
		float enemySpawnRingScale = 0.35f;
		float enemySpawnJitterScale = 0.10f;
		float enemySpawnMinPlayerDist = 1.5f;
		float enemySpawnMinEnemyDist = 0.9f;

		// Push-out resolution tuning.
		float pushSlop = 0.01f;
		float playerPushShare = 0.55f;
		float enemyPushShare = 0.45f;

		// Challenge stage tuning.
		float challengeNoDamageDuration = 18.0f;
		float challengeNoDamageAttackInterval = 1.10f;
		float challengeNoDamageTelegraph = 0.90f;
		float challengeNoDamageRadius = 1.20f;
		float challengeNoDamageDamage = 1.0f;
		int challengeNoDamageBurstCount = 2;
		float challengeDefenseDuration = 25.0f;
		float challengeDefenseBeaconMaxHp = 25.0f;
		float challengeDefenseBeaconRadius = 0.90f;
		float challengeDefenseBeaconContactDamage = 2.0f;
		float challengeDefenseSpawnInterval = 1.40f;
		float challengeDefenseEnemyMoveSpeed = 0.32f;
		float challengeDefenseSpeedHpBonusRate = 0.10f;
		float challengeDefenseRangedHpBonusRate = 0.30f;
		float challengeDefenseTankHpBonusRate = 0.50f;
		int challengeDefenseEnemyCap = 8;
	};

	/**
	 * @brief デバッグ UI や監視表示に公開する実行時状態です。
	 */
	struct GameplayDebug
	{
		// Player attack runtime state.
		int attackSwingId = 0;
		int swingHitCount = 0;
		int attackActive = 0;

		// Wave / difficulty state.
		int currentWave = 1;
		int maxWave = 1;
		int enemiesAlive = 0;
		int enemiesTarget = 0;
		int difficultyPreset = 1; // 0:Easy 1:Normal 2:Hard
		int effectiveEnemyBaseCount = 3;
		int effectiveEnemyAddPerWave = 1;
		float effectiveEnemyAttackDamage = 1.0f;
		int playerAttackDamage = 1;
		float playerAttackCooldownScale = 1.0f;
		float playerEvadeCooldownScale = 1.0f;
		float cooldownRateAttack = 1.0f;
		float cooldownRateEvade = 1.0f;
		float cooldownRateSkill1 = 1.0f;
		float cooldownRateSkill2 = 1.0f;
		int playerEvading = 0;

		// Roguelike upgrade state.
		int stageClearCount = 0;
		int attackPowerLevel = 0;
		int attackSpeedLevel = 0;
		int evadeCooldownLevel = 0;
		int lastUpgradeType = -1; // UpgradeType
		int upgradeSelectionPending = 0;
		int upgradeRerollRemain = 0;
		int upgradeOffer0 = -1;
		int upgradeOffer1 = -1;
		int upgradeOffer2 = -1;
		int rewardSelectionIndex = 0;

		// Run timer / boss request state.
		float runElapsedSec = 0.0f;
		float runRecordedSec = 0.0f;
		int runTimerRunning = 0;
		int requestBossBattle = 0;
		int bossBattleActive = 0;
		float bossHp = 0.0f;
		float bossMaxHp = 0.0f;
		float bossGuard = 0.0f;
		float bossGuardMax = 0.0f;
		int bossBroken = 0;
		int bossHpEditRequest = 0;
		int bossHpEditValue = 0;
		int bossMaxHpEditValue = 0;
		int showBossResultTimer = 0;
		char cursorHoverTarget[64] = "-";

		// Pause / title UI runtime state.
		int pauseMenuOpen = 0;
		int pauseTabIndex = 0;      // 0:Game 1:Upgrades 2:Settings
		int pauseMenuSelection = 0; // 0: Continue, 1: Option, 2: Title
		int pauseMenuRequest = 0;   // 0: None, 1: Continue, 2: Title, 3: Option
		int pauseOptionOpen = 0;
		int pauseOptionSelection = 0; // 0:Master 1:BGM 2:SE 3:Display 4:Back
		int pauseOptionRequestClose = 0;
		int titleOptionOpen = 0;
		int titleOptionSelection = 0; // 0:Master 1:BGM 2:SE 3:Display 4:KeyConfig 5:EffectDebug 6:Back
		int titleOptionRequestClose = 0;
		int titleKeyConfigOpen = 0;
		int titleKeyConfigRequestOpen = 0;
		int titleDifficultyOpen = 0;
		int titleDifficultySelection = 1; // 0:Easy 1:Normal 2:Hard
		float pauseMenuUiScale = 1.0f;
		float pauseMenuFontScale = 1.0f;
		float pauseMenuButtonScale = 1.0f;

		// Challenge runtime / debug state.
		int requestChallengeType = 0; // 0:None 1:NoDamage 2:Defense
		int challengeActive = 0;
		int challengeType = 0;
		int challengeSuccess = 0;
		int challengeRewardCount = 0;
		int challengeHitCount = 0;
		float challengeElapsedSec = 0.0f;
		float challengeDurationSec = 0.0f;
		float challengeBeaconHp = 0.0f;
		float challengeBeaconMaxHp = 0.0f;
		int challengeReturnToGameAfterReward = 0;
	};

public:
	/**
	 * @brief ローグライク強化の進行状況と選択候補を保持します。
	 */
	struct RoguelikeUpgrade
	{
		/** @brief 各強化項目の最大レベルです。 */
		static const int kLevelMax = 5;
		/** @brief 同時提示する強化候補数です。 */
		static const int kOfferCount = 3;
		/** @brief 1 run 内で進行するステージ数です。 */
		static const int kRunStageCount = 12;

		/**
		 * @brief 強化候補の種類です。
		 */
		enum UpgradeType
		{
			UpgradeAttackPower = 0,
			UpgradeAttackSpeed = 1,
			UpgradeEvadeCooldown = 2,
			UpgradeAttackPowerLarge = 3,
			UpgradeAttackSpeedLarge = 4,
			UpgradeEvadeCooldownLarge = 5,
			UpgradeSkillShot = 6,
			UpgradeSkillNova = 7,
			UpgradeSkillOrbit = 8,
			UpgradeSkillShotRange = 9,
			UpgradeSkillShotPower = 10,
			UpgradeSkillShotCooldown = 11,
			UpgradeSkillNovaRange = 12,
			UpgradeSkillNovaPower = 13,
			UpgradeSkillNovaCooldown = 14,
			UpgradeSkillOrbitRange = 15,
			UpgradeSkillOrbitCooldown = 16,
			UpgradeSkillOrbitCount = 17,
			UpgradeTypeCount = 18
		};

		/**
		 * @brief 装備スロットへ入るスキル種別です。
		 */
		enum SkillType
		{
			SkillNone = 0,
			SkillShot = 1,
			SkillNova = 2,
			SkillOrbit = 3,
			SkillTypeCount = 4
		};

		/**
		 * @brief 現在進行中の報酬フェーズです。
		 */
		enum SelectionPhase
		{
			SelectionNone = 0,
			SelectionStatus = 1,
			SelectionSkill = 2,
			SelectionMixed = 3
		};

		/**
		 * @brief run マップ上のステージ種別です。
		 */
		enum StageType
		{
			StageCombat = 0,
			StageShop = 1,
			StageRest = 2,
			StageBoss = 3
		};

		/**
		 * @brief リザルトシーン中の進行モードです。
		 */
		enum IntermissionMode
		{
			IntermissionNone = 0,
			IntermissionMapSelect = 1,
			IntermissionShop = 2,
			IntermissionRest = 3
		};

		/** @brief クリア済みステージ数です。 */
		int stageClearCount = 0;
		/** @brief 攻撃力強化レベルです。 */
		int attackPowerLevel = 0;
		/** @brief 攻撃速度強化レベルです。 */
		int attackSpeedLevel = 0;
		/** @brief 回避クールタイム短縮レベルです。 */
		int evadeCooldownLevel = 0;
		/** @brief 最後に取得した強化種別です。 */
		int lastUpgradeType = -1; // UpgradeType
		/** @brief スキルスロット1に装備しているスキルです。 */
		int skillSlot1 = SkillNone;
		/** @brief スキルスロット2に装備しているスキルです。 */
		int skillSlot2 = SkillNone;
		/** @brief 遠距離スキルの攻撃範囲レベルです。 */
		int skillShotRangeLevel = 0;
		/** @brief 遠距離スキルの攻撃倍率レベルです。 */
		int skillShotPowerLevel = 0;
		/** @brief 遠距離スキルのクールタイム短縮レベルです。 */
		int skillShotCooldownLevel = 0;
		/** @brief 近接スキルの攻撃範囲レベルです。 */
		int skillNovaRangeLevel = 0;
		/** @brief 近接スキルの攻撃倍率レベルです。 */
		int skillNovaPowerLevel = 0;
		/** @brief 近接スキルのクールタイム短縮レベルです。 */
		int skillNovaCooldownLevel = 0;
		/** @brief 衛星スキルの攻撃範囲レベルです。 */
		int skillOrbitRangeLevel = 0;
		/** @brief 衛星スキルのクールタイム短縮レベルです。 */
		int skillOrbitCooldownLevel = 0;
		/** @brief 衛星スキルの生成数レベルです。 */
		int skillOrbitCountLevel = 0;
		/** @brief 1 ステージ進行ごとに獲得するリロールアイテム数です。 */
		int rerollMaxPerStage = 1;
		/** @brief 現在所持しているリロールアイテム数です。 */
		int rerollRemain = 0;
		/** @brief 強化選択待ちかどうかです。 */
		int selectionPending = 0;
		/** @brief 現在の報酬フェーズです。 */
		int selectionPhase = SelectionNone;
		/** @brief 現在の選択フローで残っている報酬回数です。 */
		int selectionRoundsRemaining = 0;
		/** @brief 現在提示中の強化候補です。 */
		int offers[kOfferCount] =
		{
			UpgradeAttackPower,
			UpgradeAttackSpeed,
			UpgradeEvadeCooldown
		};
		/** @brief 現在進行中のステージ番号です。0 始まりです。 */
		int currentStageIndex = 0;
		/** @brief 現在進行中のステージ種別です。 */
		int currentStageType = StageCombat;
		/** @brief リザルトシーン中の進行モードです。 */
		int intermissionMode = IntermissionNone;
		/** @brief run マップ上の各ステージ種別です。 */
		int stageTypes[kRunStageCount] =
		{
			StageCombat, StageCombat, StageCombat, StageShop,
			StageCombat, StageCombat, StageCombat, StageShop,
			StageCombat, StageCombat, StageRest, StageBoss
		};
		/** @brief 各ステージへ進む際に表示する候補数です。 */
		int stageOptionCounts[kRunStageCount] =
		{
			1, 2, 2, 1, 2, 2, 2, 1, 2, 2, 1, 1
		};
		/** @brief 現在のショップ訪問で何回購入したかです。 */
		int shopPurchaseCount = 0;
	};
	/**
	 * @brief 入力デバイスごとの論理割り当てです。
	 */
	struct InputConfig
	{
		// XInput button bit values mirrored locally to avoid exposing XInput headers here.
		enum XInputButton
		{
			XInputDPadUp = 0x0001,
			XInputDPadDown = 0x0002,
			XInputDPadLeft = 0x0004,
			XInputDPadRight = 0x0008,
			XInputStart = 0x0010,
			XInputBack = 0x0020,
			XInputLeftThumb = 0x0040,
			XInputRightThumb = 0x0080,
			XInputLeftShoulder = 0x0100,
			XInputRightShoulder = 0x0200,
			XInputA = 0x1000,
			XInputB = 0x2000,
			XInputX = 0x4000,
			XInputY = 0x8000
		};

		int moveUp = 'W';
		int moveDown = 'S';
		int moveLeft = 'A';
		int moveRight = 'D';
		int dash = VK_SHIFT;
		int attack = 'F';
		int skill1 = 'O';
		int skill2 = 'P';
		int reroll = 'R';
		int xinputConfirm = XInputA;
		int xinputDash = XInputA;
		int xinputAttack = XInputB;
		int xinputSkill1 = XInputY;
		int xinputSkill2 = XInputX;
		int xinputReroll = XInputY;
		int xinputTabPrev = XInputLeftShoulder;
		int xinputTabNext = XInputRightShoulder;
		int directInputConfirm = 0;
		int directInputDash = 0;
		int directInputAttack = 1;
		int directInputSkill1 = 3;
		int directInputSkill2 = 2;
		int directInputReroll = 3;
		int directInputTabPrev = 4;
		int directInputTabNext = 5;
	};
public:
	/**
	 * @brief Transfer の唯一のインスタンスを返します。
	 * @return シングルトンインスタンスへの参照です。
	 */
	static Transfer& GetInstance()
	{
		static Transfer instance;
		return instance;
	}

	/**
	 * @brief ゲームプレイ調整値を既定値へ戻します。
	 */
	void ResetGameplayTuningToDefault();

	/**
	 * @brief ローグライク強化状態を初期値へ戻します。
	 */
	void ResetRoguelikeUpgrade();

	/**
	 * @brief キーコンフィグを既定値へ戻します。
	 */
	void ResetInputConfigToDefault();

	/**
	 * @brief 難易度プリセットに応じて基準調整値を設定します。
	 * @param preset 0:Easy 1:Normal 2:Hard の難易度です。
	 */
	void ApplyDifficultyPreset(int preset);

	/**
	 * @brief ステージクリア時の強化を自動適用します。
	 */
	void ApplyStageClearUpgrade();

	/**
	 * @brief 三択強化候補の生成を開始します。
	 */
	void BeginUpgradeSelection();

	/**
	 * @brief 挑戦ステージ用の混合報酬選択を開始します。
	 * @param rewardCount 付与する報酬回数です。
	 */
	void BeginChallengeRewardSelection(int rewardCount);

	/**
	 * @brief 現在の強化候補を再抽選します。
	 * @return 再抽選に成功した場合は true です。
	 */
	bool RerollUpgradeSelection();

	/**
	 * @brief 報酬フェーズを次段階へ進めます。
	 * @return フェーズ遷移または終了処理を行えた場合は true です。
	 */
	bool AdvanceUpgradeSelectionPhase();

	/**
	 * @brief 指定した候補番号の強化を適用します。
	 * @param offerIndex 選択した候補の添字です。
	 * @return 適用に成功した場合は true です。
	 */
	bool ApplyUpgradeSelection(int offerIndex);

	/**
	 * @brief 現在の報酬状態を正規化し、必要なら次フェーズ候補を生成します。
	 * @return 強化選択待ち状態なら true です。
	 */
	bool RefreshUpgradeSelectionState();

	/**
	 * @brief 現在フェーズに有効な候補があるか返します。
	 * @return 1件以上候補があれば true です。
	 */
	bool HasAnyUpgradeOffer() const;

	/**
	 * @brief 強化選択状態を終了し、候補と選択位置を初期化します。
	 */
	void FinishUpgradeSelection();

	/**
	 * @brief run マップ上の総ステージ数を返します。
	 * @return 固定 8 ステージです。
	 */
	int GetRunStageCount() const;

	/**
	 * @brief 指定番号のステージ種別を返します。
	 * @param stageIndex 0 始まりのステージ番号です。
	 * @return StageType の値です。
	 */
	int GetRunStageTypeAt(int stageIndex) const;

	/**
	 * @brief 現在進行中のステージ種別を返します。
	 * @return StageType の値です。
	 */
	int GetCurrentRunStageType() const;

	/**
	 * @brief 指定ステージへ進む際に表示する候補数を返します。
	 * @param stageIndex 0 始まりのステージ番号です。
	 * @return 1 から 3 の候補数です。
	 */
	int GetRunStageOptionCount(int stageIndex) const;

	/**
	 * @brief 次ステージ選択へ進行します。
	 * @return 次ステージ選択を開始できた場合は true です。
	 */
	bool BeginNextStageSelection();

	/**
	 * @brief 次ステージ候補から 1 つ選択して進行します。
	 * @param optionIndex 選択した候補番号です。
	 * @return 進行に成功した場合は true です。
	 */
	bool SelectNextStage(int optionIndex);

	/**
	 * @brief 現在の非戦闘ステージを終了し、次の進行へ移ります。
	 * @return 次の進行へ移れた場合は true です。
	 */
	bool ContinueFromCurrentNonCombatStage();

	/**
	 * @brief ステージ進行報酬としてリロールアイテムを加算します。
	 */
	void GrantStageProgressRerollItems();

	/**
	 * @brief 指定フェーズ向けの候補を生成します。
	 * @param selectionPhase SelectionPhase の値です。
	 * @param offers 候補出力先です。
	 * @return 1 件以上候補がある場合は true です。
	 */
	bool GenerateOffersForSelectionPhase(int selectionPhase, int offers[RoguelikeUpgrade::kOfferCount]) const;

	/**
	 * @brief 指定強化を即時適用します。
	 * @param upgradeType UpgradeType の値です。
	 * @return 適用に成功した場合は true です。
	 */
	bool ApplyUpgradeTypeImmediate(int upgradeType);

	/**
	 * @brief ショップ系処理向けに、指定カテゴリの強化選択画面を開始します。
	 * @param selectionPhase SelectionStatus または SelectionSkill です。
	 * @param cost 消費するリロールアイテム数です。
	 * @return 購入処理と選択開始に成功した場合は true です。
	 */
	bool PurchaseRandomUpgrade(int selectionPhase, int cost);

	/**
	 * @brief 難易度値を有効範囲へ丸めます。
	 * @param preset 補正対象の難易度値です。
	 * @return 0 から 2 に丸めた難易度値です。
	 */
	int NormalizeDifficultyPreset(int preset) const;

	/**
	 * @brief 強化レベルを有効範囲へ丸めます。
	 * @param level 補正対象レベルです。
	 * @return 0 から最大値に丸めたレベルです。
	 */
	int ClampUpgradeLevel(int level) const;

	/**
	 * @brief 強化レベル上限を返します。
	 * @return 強化レベル上限です。
	 */
	int GetUpgradeLevelMax() const;

	/**
	 * @brief スキル強化レベル上限を返します。
	 * @return スキル強化レベル上限です。
	 */
	int GetSkillUpgradeLevelMax() const;

	/**
	 * @brief 強化種別と難易度から増加段階数を返します。
	 * @param upgradeType 強化種別です。
	 * @param difficultyPreset 難易度です。
	 * @return 該当強化の増加量です。
	 */
	int GetUpgradeStepForType(int upgradeType, int difficultyPreset) const;

	/**
	 * @brief 全強化レベル合計を返します。
	 * @return 攻撃力、攻撃速度、回避短縮の合計レベルです。
	 */
	int GetTotalUpgradeLevels() const;

	/**
	 * @brief 攻撃力レベルから実ダメージ値を返します。
	 * @param level 攻撃力レベルです。
	 * @return 実ダメージ値です。
	 */
	int GetPlayerAttackDamageByLevel(int level) const;

	/**
	 * @brief 攻撃速度レベルから攻撃クールタイム倍率を返します。
	 * @param level 攻撃速度レベルです。
	 * @return 攻撃クールタイム倍率です。
	 */
	float GetAttackCooldownScaleByLevel(int level) const;

	/**
	 * @brief 回避強化レベルから回避クールタイム倍率を返します。
	 * @param level 回避強化レベルです。
	 * @return 回避クールタイム倍率です。
	 */
	float GetEvadeCooldownScaleByLevel(int level) const;

	/**
	 * @brief スキル攻撃範囲レベルから範囲倍率を返します。
	 * @param level スキル攻撃範囲レベルです。
	 * @return 1.0 から 3.0 の範囲倍率です。
	 */
	float GetSkillRangeScaleByLevel(int level) const;

	/**
	 * @brief スキル攻撃倍率レベルから倍率を返します。
	 * @param level スキル攻撃倍率レベルです。
	 * @return 1.0 から 2.0 の倍率です。
	 */
	float GetSkillDamageScaleByLevel(int level) const;

	/**
	 * @brief 衛星生成数レベルから追加ダメージ倍率を返します。
	 * @param level 衛星生成数レベルです。
	 * @return 1.0 から 1.5 の倍率です。
	 */
	float GetOrbitDamageScaleByCountLevel(int level) const;

	/**
	 * @brief スキルCT短縮レベルから減少秒数を返します。
	 * @param level スキルCT短縮レベルです。
	 * @return 0.0 から 4.0 秒の減少量です。
	 */
	float GetSkillCooldownReductionByLevel(int level) const;

	/**
	 * @brief 衛星生成数レベルから生成数を返します。
	 * @param level 衛星生成数レベルです。
	 * @return 1 から 6 の生成数です。
	 */
	int GetOrbitCountByLevel(int level) const;

	/**
	 * @brief 強化進行度から通常敵 HP 補正倍率を返します。
	 * @return 敵 HP 倍率です。
	 */
	float GetEnemyHpScaleByUpgradeProgress() const;

	/**
	 * @brief 強化進行度からボス HP 補正倍率を返します。
	 * @return ボス HP 倍率です。
	 */
	float GetBossHpScaleByUpgradeProgress() const;

	/**
	 * @brief 強化進行度から通常敵攻撃補正倍率を返します。
	 * @return 敵攻撃倍率です。
	 */
	float GetEnemyAttackScaleByUpgradeProgress() const;

	/**
	 * @brief 難易度から通常敵 HP 補正倍率を返します。
	 * @param preset 難易度です。
	 * @return 通常敵 HP 倍率です。
	 */
	float GetEnemyHpScaleByDifficulty(int preset) const;

	/**
	 * @brief 難易度からボス HP 補正倍率を返します。
	 * @param preset 難易度です。
	 * @return ボス HP 倍率です。
	 */
	float GetBossHpScaleByDifficulty(int preset) const;

	/**
	 * @brief 難易度からボス行動頻度補正倍率を返します。
	 * @param preset 難易度です。
	 * @return ボスクールタイム倍率です。
	 */
	float GetBossCooldownScaleByDifficulty(int preset) const;

	/**
	 * @brief 調整値設定ファイルを読み込みます。
	 * @param path 読込元パスです。nullptr の場合は既定パスです。
	 * @return 読み込みに成功した場合は true です。
	 */
	bool LoadGameplayTuning(const char* path = nullptr);

	/**
	 * @brief 調整値設定ファイルを保存します。
	 * @param path 保存先パスです。nullptr の場合は既定パスです。
	 * @return 保存に成功した場合は true です。
	 */
	bool SaveGameplayTuning(const char* path = nullptr) const;

	/**
	 * @brief 既定の調整値設定ファイルパスを返します。
	 * @return 調整値設定ファイルパスです。
	 */
	const char* GetGameplayTuningPath() const;
public:
	PlayerInfo player;
	EnemyInfo enemy;
	DiceInfo dice;
	CameraInfo camera{ { 0.0f, 6.0f, -6.0f },{ 0.0f, 0.0f, 0.0f } };
	/** @brief ゲーム用カメラの初期値・保存値です。 */
	CameraInfo cameraGame{ { 0.0f, 6.0f, -6.0f },{ 0.0f, 0.0f, 0.0f } };
	/** @brief デバッグ用カメラの初期値・保存値です。 */
	CameraInfo cameraDebug{ { 0.0f, 10.0f, 0.001f },{ 0.0f, 0.0f, 0.0f } };
	/** @brief 現在のカメラモードです。 */
	int cameraMode = 0;
	ObjectfromAtoB obj;
	UIInfo diceui;
	DirectX::XMFLOAT2 mousePos;
	UIobj yukari;
	UIobj fuki;
	Tyabudai tyabu;
	Tyabudai tyawan;
	ModelEditer modelediter;
	Arrow arrow;
	/** @brief 実行中に変更可能なゲームプレイ調整値です。 */
	GameplayTuning gameplay;
	/** @brief デバッグ UI へ公開する実行時状態です。 */
	GameplayDebug gameplayDebug;
	/** @brief ローグライク強化の進行状況です。 */
	RoguelikeUpgrade roguelike;
	/** @brief 実行中に変更可能な入力割り当てです。 */
	InputConfig input;
};
