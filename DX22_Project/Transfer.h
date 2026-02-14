#pragma once

#include <DirectXMath.h>
#include "Defines.h"

#define TRAN_INS Transfer &tran = Transfer::GetInstance();
#define TRAN_INS_Get Transfer &tran = Transfer::GetInstance();tran

class Transfer
{
private:
	Transfer() = default;
	~Transfer() = default;

	struct CameraInfo
	{
		DirectX::XMFLOAT3 eye;
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
	struct GameplayTuning
	{
		int enemyCount = 3;
		int waveMax = 3;
		int waveEnemyAddPerWave = 1;

		float attackWindup = 0.04f;
		float attackDuration = 0.12f;
		float attackRecovery = 0.10f;
		float attackCooldown = 0.24f;
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
		float playerDamageFlashScale = 1.65f;
		float enemyDefeatFlash = 0.28f;
		float enemyDefeatFlashScale = 1.60f;
		float volumeMaster = 1.0f;
		float volumeBgm = 0.7f;
		float volumeSe = 1.0f;

		float enemyAttackWindup = 0.55f;
		float enemyAttackCooldown = 1.00f;
		float enemyAttackRangeMin = 0.8f;
		float enemyAttackRangeScale = 1.35f;
		float enemyAttackDamage = 1.0f;
		float enemyMoveSpeed = 1.2f;
		float waveEnemyMoveSpeedAdd = 0.15f;
		float waveEnemyAttackDamageScalePerWave = 0.20f;
		float enemySeparationRadius = 1.1f;
		float enemySeparationWeight = 0.8f;
		float enemySeparationMaxOffset = 0.8f;
		float enemySpawnRingScale = 0.35f;
		float enemySpawnJitterScale = 0.10f;
		float enemySpawnMinPlayerDist = 1.5f;
		float enemySpawnMinEnemyDist = 0.9f;

		float pushSlop = 0.01f;
		float playerPushShare = 0.55f;
		float enemyPushShare = 0.45f;
	};
	struct GameplayDebug
	{
		int attackSwingId = 0;
		int swingHitCount = 0;
		int attackActive = 0;
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
		int playerEvading = 0;
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
	};
	struct RoguelikeUpgrade
	{
		static const int kOfferCount = 3;
		enum UpgradeType
		{
			UpgradeAttackPower = 0,
			UpgradeAttackSpeed = 1,
			UpgradeEvadeCooldown = 2,
			UpgradeAttackPowerLarge = 3,
			UpgradeAttackSpeedLarge = 4,
			UpgradeEvadeCooldownLarge = 5,
			UpgradeTypeCount = 6
		};

		int stageClearCount = 0;
		int attackPowerLevel = 0;
		int attackSpeedLevel = 0;
		int evadeCooldownLevel = 0;
		int lastUpgradeType = -1; // UpgradeType
		int rerollMaxPerStage = 2;
		int rerollRemain = 0;
		int selectionPending = 0;
		int offers[kOfferCount] =
		{
			UpgradeAttackPower,
			UpgradeAttackSpeed,
			UpgradeEvadeCooldown
		};
	};
public:
	static Transfer& GetInstance()
	{
		static Transfer instance;
		return instance;
	}
	void ResetGameplayTuningToDefault();
	void ResetRoguelikeUpgrade();
	void ApplyStageClearUpgrade();
	void BeginUpgradeSelection();
	bool RerollUpgradeSelection();
	bool ApplyUpgradeSelection(int offerIndex);
	bool LoadGameplayTuning(const char* path = nullptr);
	bool SaveGameplayTuning(const char* path = nullptr) const;
	const char* GetGameplayTuningPath() const;
public:
	PlayerInfo player;
	EnemyInfo enemy;
	DiceInfo dice;
	CameraInfo camera{ { 0.0f, 6.0f, -6.0f },{ 0.0f, 0.0f, 0.0f } };
	CameraInfo cameraGame{ { 0.0f, 6.0f, -6.0f },{ 0.0f, 0.0f, 0.0f } };
	CameraInfo cameraDebug{ { 0.0f, 10.0f, 0.001f },{ 0.0f, 0.0f, 0.0f } };
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
	GameplayTuning gameplay;
	GameplayDebug gameplayDebug;
	RoguelikeUpgrade roguelike;
};
