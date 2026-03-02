#include "Transfer.h"
#include <fstream>
#include <string>
#include <cstdlib>

namespace
{
	const char* kDefaultGameplayTuningPath = "Assets/gameplay_tuning.cfg";
	const char* kMirrorGameplayTuningPath = "DX22_Project/Assets/gameplay_tuning.cfg";
	const char* kDebugGameplayTuningPath = "x64/Debug/Assets/gameplay_tuning.cfg";
	const char* kUpstreamMirrorGameplayTuningPath = "../../DX22_Project/Assets/gameplay_tuning.cfg";

	bool IsDefaultPathArgument(const char* path)
	{
		return !(path && path[0] != '\0');
	}

	const char* ResolvePath(const char* path)
	{
		if (!IsDefaultPathArgument(path))
		{
			return path;
		}
		return kDefaultGameplayTuningPath;
	}

	void Trim(std::string& s)
	{
		size_t begin = 0;
		while (begin < s.size() && (s[begin] == ' ' || s[begin] == '\t' || s[begin] == '\r' || s[begin] == '\n'))
		{
			++begin;
		}

		size_t end = s.size();
		while (end > begin && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n'))
		{
			--end;
		}

		s = s.substr(begin, end - begin);
	}

	int ToInt(const std::string& s, int fallback)
	{
		char* endPtr = nullptr;
		const long v = std::strtol(s.c_str(), &endPtr, 10);
		if (endPtr == s.c_str())
		{
			return fallback;
		}
		return static_cast<int>(v);
	}

	float ToFloat(const std::string& s, float fallback)
	{
		char* endPtr = nullptr;
		const float v = std::strtof(s.c_str(), &endPtr);
		if (endPtr == s.c_str())
		{
			return fallback;
		}
		return v;
	}

	int ClampInt(int v, int lo, int hi)
	{
		if (v < lo) return lo;
		if (v > hi) return hi;
		return v;
	}

	const int kUpgradeTierMax = 10;

	int ClampUpgradeTier(int level)
	{
		return ClampInt(level, 0, kUpgradeTierMax);
	}

	const int kAttackDamageByTier[kUpgradeTierMax + 1] =
	{
		1, 2, 2, 3, 3, 4, 5, 6, 7, 8, 10
	};

	const float kAttackCooldownScaleByTier[kUpgradeTierMax + 1] =
	{
		1.00f, 0.96f, 0.92f, 0.88f, 0.84f, 0.80f, 0.76f, 0.72f, 0.68f, 0.64f, 0.60f
	};

	const float kEvadeCooldownScaleByTier[kUpgradeTierMax + 1] =
	{
		1.00f, 0.95f, 0.90f, 0.84f, 0.78f, 0.72f, 0.66f, 0.60f, 0.54f, 0.48f, 0.42f
	};

	int RandRangeInt(int minValue, int maxValue)
	{
		if (maxValue <= minValue) return minValue;
		const int span = maxValue - minValue + 1;
		return minValue + (std::rand() % span);
	}

	const int kUpgradeOfferCount = 3;
	const int kUpgradeTypeCount = 6;
	const int kUpgradeOfferNone = -1;

	bool IsUpgradeTypeAvailable(int upgradeType, int attackPowerLevel, int attackSpeedLevel, int evadeCooldownLevel)
	{
		switch (upgradeType)
		{
		case 0:
		case 3:
			return attackPowerLevel < kUpgradeTierMax;
		case 1:
		case 4:
			return attackSpeedLevel < kUpgradeTierMax;
		case 2:
		case 5:
			return evadeCooldownLevel < kUpgradeTierMax;
		default:
			return false;
		}
	}

	void GenerateUpgradeOffers(int offers[kUpgradeOfferCount], int attackPowerLevel, int attackSpeedLevel, int evadeCooldownLevel)
	{
		int available[kUpgradeTypeCount]{};
		int availableCount = 0;
		for (int t = 0; t < kUpgradeTypeCount; ++t)
		{
			if (IsUpgradeTypeAvailable(t, attackPowerLevel, attackSpeedLevel, evadeCooldownLevel))
			{
				available[availableCount++] = t;
			}
		}

		if (availableCount == 0)
		{
			for (int i = 0; i < kUpgradeOfferCount; ++i)
			{
				offers[i] = kUpgradeOfferNone;
			}
			return;
		}

		for (int i = 0; i < availableCount; ++i)
		{
			const int j = RandRangeInt(i, availableCount - 1);
			const int tmp = available[i];
			available[i] = available[j];
			available[j] = tmp;
		}

		for (int i = 0; i < kUpgradeOfferCount; ++i)
		{
			if (i < availableCount)
			{
				offers[i] = available[i];
			}
			else
			{
				offers[i] = available[RandRangeInt(0, availableCount - 1)];
			}
		}
	}

	void ApplyUpgradeType(int& attackPowerLevel, int& attackSpeedLevel, int& evadeCooldownLevel, int upgradeType)
	{
		switch (upgradeType)
		{
		case 0:
			attackPowerLevel = ClampUpgradeTier(attackPowerLevel + 1);
			break;
		case 1:
			attackSpeedLevel = ClampUpgradeTier(attackSpeedLevel + 1);
			break;
		case 2:
			evadeCooldownLevel = ClampUpgradeTier(evadeCooldownLevel + 1);
			break;
		case 3:
			attackPowerLevel = ClampUpgradeTier(attackPowerLevel + 2);
			break;
		case 4:
			attackSpeedLevel = ClampUpgradeTier(attackSpeedLevel + 2);
			break;
		case 5:
			evadeCooldownLevel = ClampUpgradeTier(evadeCooldownLevel + 2);
			break;
		default:
			break;
		}
	}
}

void Transfer::ResetGameplayTuningToDefault()
{
	gameplay = GameplayTuning{};
}

void Transfer::ResetRoguelikeUpgrade()
{
	roguelike = RoguelikeUpgrade{};
}

void Transfer::ApplyStageClearUpgrade()
{
	const int nextType = roguelike.stageClearCount % 3;
	++roguelike.stageClearCount;
	roguelike.lastUpgradeType = nextType;
	ApplyUpgradeType(roguelike.attackPowerLevel, roguelike.attackSpeedLevel, roguelike.evadeCooldownLevel, nextType);
}

void Transfer::BeginUpgradeSelection()
{
	roguelike.selectionPending = 1;
	roguelike.rerollRemain = ClampInt(roguelike.rerollMaxPerStage, 0, 99);
	GenerateUpgradeOffers(roguelike.offers, roguelike.attackPowerLevel, roguelike.attackSpeedLevel, roguelike.evadeCooldownLevel);
}

bool Transfer::RerollUpgradeSelection()
{
	if (roguelike.selectionPending == 0) return false;
	if (roguelike.rerollRemain <= 0) return false;
	--roguelike.rerollRemain;
	GenerateUpgradeOffers(roguelike.offers, roguelike.attackPowerLevel, roguelike.attackSpeedLevel, roguelike.evadeCooldownLevel);
	return true;
}

bool Transfer::ApplyUpgradeSelection(int offerIndex)
{
	if (roguelike.selectionPending == 0) return false;
	if (offerIndex < 0 || offerIndex >= RoguelikeUpgrade::kOfferCount) return false;

	const int selectedType = roguelike.offers[offerIndex];
	if (selectedType < 0 || selectedType >= RoguelikeUpgrade::UpgradeTypeCount) return false;
	ApplyUpgradeType(roguelike.attackPowerLevel, roguelike.attackSpeedLevel, roguelike.evadeCooldownLevel, selectedType);
	++roguelike.stageClearCount;
	roguelike.lastUpgradeType = selectedType;
	roguelike.selectionPending = 0;
	roguelike.rerollRemain = 0;
	return true;
}

int Transfer::ClampUpgradeLevel(int level) const
{
	return ClampUpgradeTier(level);
}

int Transfer::GetUpgradeLevelMax() const
{
	return RoguelikeUpgrade::kLevelMax;
}

int Transfer::GetPlayerAttackDamageByLevel(int level) const
{
	return kAttackDamageByTier[ClampUpgradeTier(level)];
}

float Transfer::GetAttackCooldownScaleByLevel(int level) const
{
	return kAttackCooldownScaleByTier[ClampUpgradeTier(level)];
}

float Transfer::GetEvadeCooldownScaleByLevel(int level) const
{
	return kEvadeCooldownScaleByTier[ClampUpgradeTier(level)];
}


const char* Transfer::GetGameplayTuningPath() const
{
	return kDefaultGameplayTuningPath;
}

bool Transfer::LoadGameplayTuning(const char* path)
{
	const char* resolvedPath = ResolvePath(path);
	std::ifstream ifs(resolvedPath);
	if (!ifs.is_open() && IsDefaultPathArgument(path))
	{
		const char* fallbackPaths[] =
		{
			kDebugGameplayTuningPath,
			kMirrorGameplayTuningPath,
			kUpstreamMirrorGameplayTuningPath
		};
		for (const char* fallback : fallbackPaths)
		{
			ifs.clear();
			ifs.open(fallback);
			if (ifs.is_open())
			{
				resolvedPath = fallback;
				break;
			}
		}
	}

	if (!ifs.is_open())
	{
		return false;
	}

	GameplayTuning loaded{};
	int loadedPreset = gameplayDebug.difficultyPreset;
	RoguelikeUpgrade loadedRogue = roguelike;

	std::string line;
	while (std::getline(ifs, line))
	{
		Trim(line);
		if (line.empty()) continue;
		if (line[0] == '#') continue;

		const size_t sep = line.find('=');
		if (sep == std::string::npos) continue;

		std::string key = line.substr(0, sep);
		std::string value = line.substr(sep + 1);
		Trim(key);
		Trim(value);
		if (key.empty() || value.empty()) continue;

		if (key == "enemyCount") loaded.enemyCount = ToInt(value, loaded.enemyCount);
		else if (key == "waveMax") loaded.waveMax = ToInt(value, loaded.waveMax);
		else if (key == "waveEnemyAddPerWave") loaded.waveEnemyAddPerWave = ToInt(value, loaded.waveEnemyAddPerWave);
		else if (key == "cameraIntroDuration") loaded.cameraIntroDuration = ToFloat(value, loaded.cameraIntroDuration);
		else if (key == "cameraIntroFocusDistance") loaded.cameraIntroFocusDistance = ToFloat(value, loaded.cameraIntroFocusDistance);
		else if (key == "attackWindup") loaded.attackWindup = ToFloat(value, loaded.attackWindup);
		else if (key == "attackDuration") loaded.attackDuration = ToFloat(value, loaded.attackDuration);
		else if (key == "attackRecovery") loaded.attackRecovery = ToFloat(value, loaded.attackRecovery);
		else if (key == "attackCooldown") loaded.attackCooldown = ToFloat(value, loaded.attackCooldown);
		else if (key == "skill1Cooldown") loaded.skill1Cooldown = ToFloat(value, loaded.skill1Cooldown);
		else if (key == "skill2Cooldown") loaded.skill2Cooldown = ToFloat(value, loaded.skill2Cooldown);
		else if (key == "screenShakeHitThreshold") loaded.screenShakeHitThreshold = ToInt(value, loaded.screenShakeHitThreshold);
		else if (key == "screenShakeDuration") loaded.screenShakeDuration = ToFloat(value, loaded.screenShakeDuration);
		else if (key == "screenShakeAmplitude") loaded.screenShakeAmplitude = ToFloat(value, loaded.screenShakeAmplitude);
		else if (key == "attackSweepDegrees") loaded.attackSweepDegrees = ToFloat(value, loaded.attackSweepDegrees);
		else if (key == "attackSweepRadiusScale") loaded.attackSweepRadiusScale = ToFloat(value, loaded.attackSweepRadiusScale);
		else if (key == "attackWidthScale") loaded.attackWidthScale = ToFloat(value, loaded.attackWidthScale);
		else if (key == "attackDepthScale") loaded.attackDepthScale = ToFloat(value, loaded.attackDepthScale);
		else if (key == "attackHitStop") loaded.attackHitStop = ToFloat(value, loaded.attackHitStop);
		else if (key == "attackKnockback") loaded.attackKnockback = ToFloat(value, loaded.attackKnockback);
		else if (key == "attackHitFlash") loaded.attackHitFlash = ToFloat(value, loaded.attackHitFlash);
		else if (key == "attackTrailInterval") loaded.attackTrailInterval = ToFloat(value, loaded.attackTrailInterval);
		else if (key == "attackTrailLife") loaded.attackTrailLife = ToFloat(value, loaded.attackTrailLife);
		else if (key == "attackTrailScale") loaded.attackTrailScale = ToFloat(value, loaded.attackTrailScale);
		else if (key == "playerDamageFlash") loaded.playerDamageFlash = ToFloat(value, loaded.playerDamageFlash);
		else if (key == "playerDamageInvincible") loaded.playerDamageInvincible = ToFloat(value, loaded.playerDamageInvincible);
		else if (key == "playerDamageFlashScale") loaded.playerDamageFlashScale = ToFloat(value, loaded.playerDamageFlashScale);
		else if (key == "enemyDefeatFlash") loaded.enemyDefeatFlash = ToFloat(value, loaded.enemyDefeatFlash);
		else if (key == "enemyDefeatFlashScale") loaded.enemyDefeatFlashScale = ToFloat(value, loaded.enemyDefeatFlashScale);
		else if (key == "volumeMaster") loaded.volumeMaster = ToFloat(value, loaded.volumeMaster);
		else if (key == "volumeBgm") loaded.volumeBgm = ToFloat(value, loaded.volumeBgm);
		else if (key == "volumeSe") loaded.volumeSe = ToFloat(value, loaded.volumeSe);
		else if (key == "directionMarkerAlpha") loaded.directionMarkerAlpha = ToFloat(value, loaded.directionMarkerAlpha);
		else if (key == "directionMarkerOverlapAlpha") loaded.directionMarkerOverlapAlpha = ToFloat(value, loaded.directionMarkerOverlapAlpha);
		else if (key == "bossHpBarWidthRate") loaded.bossHpBarWidthRate = ToFloat(value, loaded.bossHpBarWidthRate);
		else if (key == "bossHpBarHeightRate") loaded.bossHpBarHeightRate = ToFloat(value, loaded.bossHpBarHeightRate);
		else if (key == "bossSizeAreaScale") loaded.bossSizeAreaScale = ToFloat(value, loaded.bossSizeAreaScale);
		else if (key == "bossMaxHp") loaded.bossMaxHp = ToInt(value, loaded.bossMaxHp);
		else if (key == "bossAttackTelegraph") loaded.bossAttackTelegraph = ToFloat(value, loaded.bossAttackTelegraph);
		else if (key == "bossAttackJumpOutTime") loaded.bossAttackJumpOutTime = ToFloat(value, loaded.bossAttackJumpOutTime);
		else if (key == "bossAttackDashDuration") loaded.bossAttackDashDuration = ToFloat(value, loaded.bossAttackDashDuration);
		else if (key == "bossAttackCooldown") loaded.bossAttackCooldown = ToFloat(value, loaded.bossAttackCooldown);
		else if (key == "bossAttackLanePlayerScale") loaded.bossAttackLanePlayerScale = ToFloat(value, loaded.bossAttackLanePlayerScale);
		else if (key == "bossAttackDamage") loaded.bossAttackDamage = ToFloat(value, loaded.bossAttackDamage);
		else if (key == "bossDashNarrowTelegraph") loaded.bossDashNarrowTelegraph = ToFloat(value, loaded.bossDashNarrowTelegraph);
		else if (key == "bossDashWideTelegraph") loaded.bossDashWideTelegraph = ToFloat(value, loaded.bossDashWideTelegraph);
		else if (key == "bossDashWideWidthRate") loaded.bossDashWideWidthRate = ToFloat(value, loaded.bossDashWideWidthRate);
		else if (key == "bossRandomRainCount") loaded.bossRandomRainCount = ToInt(value, loaded.bossRandomRainCount);
		else if (key == "bossRandomRainTelegraph") loaded.bossRandomRainTelegraph = ToFloat(value, loaded.bossRandomRainTelegraph);
		else if (key == "bossRandomRainRadiusScale") loaded.bossRandomRainRadiusScale = ToFloat(value, loaded.bossRandomRainRadiusScale);
		else if (key == "bossSummonMin") loaded.bossSummonMin = ToInt(value, loaded.bossSummonMin);
		else if (key == "bossSummonMax") loaded.bossSummonMax = ToInt(value, loaded.bossSummonMax);
		else if (key == "bossSummonTelegraph") loaded.bossSummonTelegraph = ToFloat(value, loaded.bossSummonTelegraph);
		else if (key == "bossTrackingDropCount") loaded.bossTrackingDropCount = ToInt(value, loaded.bossTrackingDropCount);
		else if (key == "bossTrackingDropTelegraph") loaded.bossTrackingDropTelegraph = ToFloat(value, loaded.bossTrackingDropTelegraph);
		else if (key == "bossTrackingDropRadiusScale") loaded.bossTrackingDropRadiusScale = ToFloat(value, loaded.bossTrackingDropRadiusScale);
		else if (key == "bossUltimateCrossTelegraph") loaded.bossUltimateCrossTelegraph = ToFloat(value, loaded.bossUltimateCrossTelegraph);
		else if (key == "bossUltimateCrossLaneScale") loaded.bossUltimateCrossLaneScale = ToFloat(value, loaded.bossUltimateCrossLaneScale);
		else if (key == "bossUltimateStompCount") loaded.bossUltimateStompCount = ToInt(value, loaded.bossUltimateStompCount);
		else if (key == "bossUltimateStompTelegraph") loaded.bossUltimateStompTelegraph = ToFloat(value, loaded.bossUltimateStompTelegraph);
		else if (key == "bossUltimateStompRadiusScale") loaded.bossUltimateStompRadiusScale = ToFloat(value, loaded.bossUltimateStompRadiusScale);
		else if (key == "bossUltimateFieldTelegraph") loaded.bossUltimateFieldTelegraph = ToFloat(value, loaded.bossUltimateFieldTelegraph);
		else if (key == "bossUltimateFieldSafeScale") loaded.bossUltimateFieldSafeScale = ToFloat(value, loaded.bossUltimateFieldSafeScale);
		else if (key == "enemyAttackWindup") loaded.enemyAttackWindup = ToFloat(value, loaded.enemyAttackWindup);
		else if (key == "enemyAttackCooldown") loaded.enemyAttackCooldown = ToFloat(value, loaded.enemyAttackCooldown);
		else if (key == "enemyAttackRangeMin") loaded.enemyAttackRangeMin = ToFloat(value, loaded.enemyAttackRangeMin);
		else if (key == "enemyAttackRangeScale") loaded.enemyAttackRangeScale = ToFloat(value, loaded.enemyAttackRangeScale);
		else if (key == "enemyAttackDamage") loaded.enemyAttackDamage = ToFloat(value, loaded.enemyAttackDamage);
		else if (key == "enemyMoveSpeed") loaded.enemyMoveSpeed = ToFloat(value, loaded.enemyMoveSpeed);
		else if (key == "waveEnemyMoveSpeedAdd") loaded.waveEnemyMoveSpeedAdd = ToFloat(value, loaded.waveEnemyMoveSpeedAdd);
		else if (key == "waveEnemyAttackDamageScalePerWave") loaded.waveEnemyAttackDamageScalePerWave = ToFloat(value, loaded.waveEnemyAttackDamageScalePerWave);
		else if (key == "enemyProjectileSpeed") loaded.enemyProjectileSpeed = ToFloat(value, loaded.enemyProjectileSpeed);
		else if (key == "enemyProjectileLife") loaded.enemyProjectileLife = ToFloat(value, loaded.enemyProjectileLife);
		else if (key == "enemyProjectileRadius") loaded.enemyProjectileRadius = ToFloat(value, loaded.enemyProjectileRadius);
		else if (key == "enemyProjectileDamageScale") loaded.enemyProjectileDamageScale = ToFloat(value, loaded.enemyProjectileDamageScale);
		else if (key == "enemySeparationRadius") loaded.enemySeparationRadius = ToFloat(value, loaded.enemySeparationRadius);
		else if (key == "enemySeparationWeight") loaded.enemySeparationWeight = ToFloat(value, loaded.enemySeparationWeight);
		else if (key == "enemySeparationMaxOffset") loaded.enemySeparationMaxOffset = ToFloat(value, loaded.enemySeparationMaxOffset);
		else if (key == "enemySpawnRingScale") loaded.enemySpawnRingScale = ToFloat(value, loaded.enemySpawnRingScale);
		else if (key == "enemySpawnJitterScale") loaded.enemySpawnJitterScale = ToFloat(value, loaded.enemySpawnJitterScale);
		else if (key == "enemySpawnMinPlayerDist") loaded.enemySpawnMinPlayerDist = ToFloat(value, loaded.enemySpawnMinPlayerDist);
		else if (key == "enemySpawnMinEnemyDist") loaded.enemySpawnMinEnemyDist = ToFloat(value, loaded.enemySpawnMinEnemyDist);
		else if (key == "pushSlop") loaded.pushSlop = ToFloat(value, loaded.pushSlop);
		else if (key == "playerPushShare") loaded.playerPushShare = ToFloat(value, loaded.playerPushShare);
		else if (key == "enemyPushShare") loaded.enemyPushShare = ToFloat(value, loaded.enemyPushShare);
		else if (key == "difficultyPreset") loadedPreset = ToInt(value, loadedPreset);
		else if (key == "stageClearCount") loadedRogue.stageClearCount = ToInt(value, loadedRogue.stageClearCount);
		else if (key == "attackPowerLevel") loadedRogue.attackPowerLevel = ToInt(value, loadedRogue.attackPowerLevel);
		else if (key == "attackSpeedLevel") loadedRogue.attackSpeedLevel = ToInt(value, loadedRogue.attackSpeedLevel);
		else if (key == "evadeCooldownLevel") loadedRogue.evadeCooldownLevel = ToInt(value, loadedRogue.evadeCooldownLevel);
		else if (key == "lastUpgradeType") loadedRogue.lastUpgradeType = ToInt(value, loadedRogue.lastUpgradeType);
		else if (key == "upgradeRerollMax") loadedRogue.rerollMaxPerStage = ToInt(value, loadedRogue.rerollMaxPerStage);
	}

	loaded.screenShakeHitThreshold = ClampInt(loaded.screenShakeHitThreshold, 1, 16);
	if (loaded.cameraIntroDuration < 0.10f) loaded.cameraIntroDuration = 0.10f;
	if (loaded.cameraIntroFocusDistance < 0.50f) loaded.cameraIntroFocusDistance = 0.50f;
	if (loaded.playerDamageInvincible < 0.0f) loaded.playerDamageInvincible = 0.0f;
	if (loaded.screenShakeDuration < 0.0f) loaded.screenShakeDuration = 0.0f;
	if (loaded.screenShakeAmplitude < 0.0f) loaded.screenShakeAmplitude = 0.0f;
	if (loaded.directionMarkerAlpha < 0.0f) loaded.directionMarkerAlpha = 0.0f;
	if (loaded.directionMarkerAlpha > 1.0f) loaded.directionMarkerAlpha = 1.0f;
	if (loaded.directionMarkerOverlapAlpha < 0.0f) loaded.directionMarkerOverlapAlpha = 0.0f;
	if (loaded.directionMarkerOverlapAlpha > 1.0f) loaded.directionMarkerOverlapAlpha = 1.0f;
	if (loaded.bossHpBarWidthRate < 0.20f) loaded.bossHpBarWidthRate = 0.20f;
	if (loaded.bossHpBarWidthRate > 0.90f) loaded.bossHpBarWidthRate = 0.90f;
	if (loaded.bossHpBarHeightRate < 0.01f) loaded.bossHpBarHeightRate = 0.01f;
	if (loaded.bossHpBarHeightRate > 0.20f) loaded.bossHpBarHeightRate = 0.20f;
	if (loaded.bossSizeAreaScale < 4.0f) loaded.bossSizeAreaScale = 4.0f;
	if (loaded.bossSizeAreaScale > 12.0f) loaded.bossSizeAreaScale = 12.0f;
	loaded.bossMaxHp = ClampInt(loaded.bossMaxHp, 1, 9999);
	if (loaded.bossAttackTelegraph < 0.10f) loaded.bossAttackTelegraph = 0.10f;
	if (loaded.bossAttackJumpOutTime < 0.0f) loaded.bossAttackJumpOutTime = 0.0f;
	if (loaded.bossAttackDashDuration < 0.05f) loaded.bossAttackDashDuration = 0.05f;
	if (loaded.bossAttackCooldown < 0.0f) loaded.bossAttackCooldown = 0.0f;
	if (loaded.bossAttackLanePlayerScale < 0.5f) loaded.bossAttackLanePlayerScale = 0.5f;
	if (loaded.bossAttackLanePlayerScale > 8.0f) loaded.bossAttackLanePlayerScale = 8.0f;
	if (loaded.bossAttackDamage < 0.0f) loaded.bossAttackDamage = 0.0f;
	if (loaded.bossDashNarrowTelegraph < 0.10f) loaded.bossDashNarrowTelegraph = 0.10f;
	if (loaded.bossDashWideTelegraph < 0.10f) loaded.bossDashWideTelegraph = 0.10f;
	if (loaded.bossDashWideWidthRate < 0.10f) loaded.bossDashWideWidthRate = 0.10f;
	if (loaded.bossDashWideWidthRate > 1.00f) loaded.bossDashWideWidthRate = 1.00f;
	loaded.bossRandomRainCount = ClampInt(loaded.bossRandomRainCount, 1, 16);
	if (loaded.bossRandomRainTelegraph < 0.10f) loaded.bossRandomRainTelegraph = 0.10f;
	if (loaded.bossRandomRainRadiusScale < 0.25f) loaded.bossRandomRainRadiusScale = 0.25f;
	loaded.bossSummonMin = ClampInt(loaded.bossSummonMin, 1, 32);
	loaded.bossSummonMax = ClampInt(loaded.bossSummonMax, 1, 32);
	if (loaded.bossSummonMax < loaded.bossSummonMin) loaded.bossSummonMax = loaded.bossSummonMin;
	if (loaded.bossSummonTelegraph < 0.10f) loaded.bossSummonTelegraph = 0.10f;
	loaded.bossTrackingDropCount = ClampInt(loaded.bossTrackingDropCount, 1, 16);
	if (loaded.bossTrackingDropTelegraph < 0.10f) loaded.bossTrackingDropTelegraph = 0.10f;
	if (loaded.bossTrackingDropRadiusScale < 0.5f) loaded.bossTrackingDropRadiusScale = 0.5f;
	if (loaded.bossUltimateCrossTelegraph < 0.10f) loaded.bossUltimateCrossTelegraph = 0.10f;
	if (loaded.bossUltimateCrossLaneScale < 0.25f) loaded.bossUltimateCrossLaneScale = 0.25f;
	loaded.bossUltimateStompCount = ClampInt(loaded.bossUltimateStompCount, 1, 16);
	if (loaded.bossUltimateStompTelegraph < 0.10f) loaded.bossUltimateStompTelegraph = 0.10f;
	if (loaded.bossUltimateStompRadiusScale < 0.5f) loaded.bossUltimateStompRadiusScale = 0.5f;
	if (loaded.bossUltimateFieldTelegraph < 0.10f) loaded.bossUltimateFieldTelegraph = 0.10f;
	if (loaded.bossUltimateFieldSafeScale < 0.5f) loaded.bossUltimateFieldSafeScale = 0.5f;
	gameplay = loaded;
	loadedPreset = ClampInt(loadedPreset, 0, 2);
	gameplayDebug.difficultyPreset = loadedPreset;
	loadedRogue.stageClearCount = ClampInt(loadedRogue.stageClearCount, 0, 9999);
	loadedRogue.attackPowerLevel = ClampUpgradeTier(loadedRogue.attackPowerLevel);
	loadedRogue.attackSpeedLevel = ClampUpgradeTier(loadedRogue.attackSpeedLevel);
	loadedRogue.evadeCooldownLevel = ClampUpgradeTier(loadedRogue.evadeCooldownLevel);
	loadedRogue.lastUpgradeType = ClampInt(loadedRogue.lastUpgradeType, -1, RoguelikeUpgrade::UpgradeTypeCount - 1);
	loadedRogue.rerollMaxPerStage = ClampInt(loadedRogue.rerollMaxPerStage, 0, 9);
	loadedRogue.rerollRemain = 0;
	loadedRogue.selectionPending = 0;
	GenerateUpgradeOffers(loadedRogue.offers, loadedRogue.attackPowerLevel, loadedRogue.attackSpeedLevel, loadedRogue.evadeCooldownLevel);
	roguelike = loadedRogue;

	if (IsDefaultPathArgument(path))
	{
		// Keep runtime/source copies aligned after loading default configuration.
		SaveGameplayTuning();
	}

	return true;
}

bool Transfer::SaveGameplayTuning(const char* path) const
{
	auto writeToPath = [&](const char* targetPath) -> bool
	{
		std::ofstream ofs(targetPath, std::ios::trunc);
		if (!ofs.is_open())
		{
			return false;
		}

		ofs << "# DX22 gameplay tuning\n";
		ofs << "enemyCount=" << gameplay.enemyCount << "\n";
		ofs << "waveMax=" << gameplay.waveMax << "\n";
		ofs << "waveEnemyAddPerWave=" << gameplay.waveEnemyAddPerWave << "\n";
		ofs << "cameraIntroDuration=" << gameplay.cameraIntroDuration << "\n";
		ofs << "cameraIntroFocusDistance=" << gameplay.cameraIntroFocusDistance << "\n";
		ofs << "attackWindup=" << gameplay.attackWindup << "\n";
		ofs << "attackDuration=" << gameplay.attackDuration << "\n";
		ofs << "attackRecovery=" << gameplay.attackRecovery << "\n";
		ofs << "attackCooldown=" << gameplay.attackCooldown << "\n";
		ofs << "skill1Cooldown=" << gameplay.skill1Cooldown << "\n";
		ofs << "skill2Cooldown=" << gameplay.skill2Cooldown << "\n";
		ofs << "screenShakeHitThreshold=" << gameplay.screenShakeHitThreshold << "\n";
		ofs << "screenShakeDuration=" << gameplay.screenShakeDuration << "\n";
		ofs << "screenShakeAmplitude=" << gameplay.screenShakeAmplitude << "\n";
		ofs << "attackSweepDegrees=" << gameplay.attackSweepDegrees << "\n";
		ofs << "attackSweepRadiusScale=" << gameplay.attackSweepRadiusScale << "\n";
		ofs << "attackWidthScale=" << gameplay.attackWidthScale << "\n";
		ofs << "attackDepthScale=" << gameplay.attackDepthScale << "\n";
		ofs << "attackHitStop=" << gameplay.attackHitStop << "\n";
		ofs << "attackKnockback=" << gameplay.attackKnockback << "\n";
		ofs << "attackHitFlash=" << gameplay.attackHitFlash << "\n";
		ofs << "attackTrailInterval=" << gameplay.attackTrailInterval << "\n";
		ofs << "attackTrailLife=" << gameplay.attackTrailLife << "\n";
		ofs << "attackTrailScale=" << gameplay.attackTrailScale << "\n";
		ofs << "playerDamageFlash=" << gameplay.playerDamageFlash << "\n";
		ofs << "playerDamageInvincible=" << gameplay.playerDamageInvincible << "\n";
		ofs << "playerDamageFlashScale=" << gameplay.playerDamageFlashScale << "\n";
		ofs << "enemyDefeatFlash=" << gameplay.enemyDefeatFlash << "\n";
		ofs << "enemyDefeatFlashScale=" << gameplay.enemyDefeatFlashScale << "\n";
		ofs << "volumeMaster=" << gameplay.volumeMaster << "\n";
		ofs << "volumeBgm=" << gameplay.volumeBgm << "\n";
		ofs << "volumeSe=" << gameplay.volumeSe << "\n";
		ofs << "directionMarkerAlpha=" << gameplay.directionMarkerAlpha << "\n";
		ofs << "directionMarkerOverlapAlpha=" << gameplay.directionMarkerOverlapAlpha << "\n";
		ofs << "bossHpBarWidthRate=" << gameplay.bossHpBarWidthRate << "\n";
		ofs << "bossHpBarHeightRate=" << gameplay.bossHpBarHeightRate << "\n";
		ofs << "bossSizeAreaScale=" << gameplay.bossSizeAreaScale << "\n";
		ofs << "bossMaxHp=" << gameplay.bossMaxHp << "\n";
		ofs << "bossAttackTelegraph=" << gameplay.bossAttackTelegraph << "\n";
		ofs << "bossAttackJumpOutTime=" << gameplay.bossAttackJumpOutTime << "\n";
		ofs << "bossAttackDashDuration=" << gameplay.bossAttackDashDuration << "\n";
		ofs << "bossAttackCooldown=" << gameplay.bossAttackCooldown << "\n";
		ofs << "bossAttackLanePlayerScale=" << gameplay.bossAttackLanePlayerScale << "\n";
		ofs << "bossAttackDamage=" << gameplay.bossAttackDamage << "\n";
		ofs << "bossDashNarrowTelegraph=" << gameplay.bossDashNarrowTelegraph << "\n";
		ofs << "bossDashWideTelegraph=" << gameplay.bossDashWideTelegraph << "\n";
		ofs << "bossDashWideWidthRate=" << gameplay.bossDashWideWidthRate << "\n";
		ofs << "bossRandomRainCount=" << gameplay.bossRandomRainCount << "\n";
		ofs << "bossRandomRainTelegraph=" << gameplay.bossRandomRainTelegraph << "\n";
		ofs << "bossRandomRainRadiusScale=" << gameplay.bossRandomRainRadiusScale << "\n";
		ofs << "bossSummonMin=" << gameplay.bossSummonMin << "\n";
		ofs << "bossSummonMax=" << gameplay.bossSummonMax << "\n";
		ofs << "bossSummonTelegraph=" << gameplay.bossSummonTelegraph << "\n";
		ofs << "bossTrackingDropCount=" << gameplay.bossTrackingDropCount << "\n";
		ofs << "bossTrackingDropTelegraph=" << gameplay.bossTrackingDropTelegraph << "\n";
		ofs << "bossTrackingDropRadiusScale=" << gameplay.bossTrackingDropRadiusScale << "\n";
		ofs << "bossUltimateCrossTelegraph=" << gameplay.bossUltimateCrossTelegraph << "\n";
		ofs << "bossUltimateCrossLaneScale=" << gameplay.bossUltimateCrossLaneScale << "\n";
		ofs << "bossUltimateStompCount=" << gameplay.bossUltimateStompCount << "\n";
		ofs << "bossUltimateStompTelegraph=" << gameplay.bossUltimateStompTelegraph << "\n";
		ofs << "bossUltimateStompRadiusScale=" << gameplay.bossUltimateStompRadiusScale << "\n";
		ofs << "bossUltimateFieldTelegraph=" << gameplay.bossUltimateFieldTelegraph << "\n";
		ofs << "bossUltimateFieldSafeScale=" << gameplay.bossUltimateFieldSafeScale << "\n";
		ofs << "enemyAttackWindup=" << gameplay.enemyAttackWindup << "\n";
		ofs << "enemyAttackCooldown=" << gameplay.enemyAttackCooldown << "\n";
		ofs << "enemyAttackRangeMin=" << gameplay.enemyAttackRangeMin << "\n";
		ofs << "enemyAttackRangeScale=" << gameplay.enemyAttackRangeScale << "\n";
		ofs << "enemyAttackDamage=" << gameplay.enemyAttackDamage << "\n";
		ofs << "enemyMoveSpeed=" << gameplay.enemyMoveSpeed << "\n";
		ofs << "waveEnemyMoveSpeedAdd=" << gameplay.waveEnemyMoveSpeedAdd << "\n";
		ofs << "waveEnemyAttackDamageScalePerWave=" << gameplay.waveEnemyAttackDamageScalePerWave << "\n";
		ofs << "enemyProjectileSpeed=" << gameplay.enemyProjectileSpeed << "\n";
		ofs << "enemyProjectileLife=" << gameplay.enemyProjectileLife << "\n";
		ofs << "enemyProjectileRadius=" << gameplay.enemyProjectileRadius << "\n";
		ofs << "enemyProjectileDamageScale=" << gameplay.enemyProjectileDamageScale << "\n";
		ofs << "enemySeparationRadius=" << gameplay.enemySeparationRadius << "\n";
		ofs << "enemySeparationWeight=" << gameplay.enemySeparationWeight << "\n";
		ofs << "enemySeparationMaxOffset=" << gameplay.enemySeparationMaxOffset << "\n";
		ofs << "enemySpawnRingScale=" << gameplay.enemySpawnRingScale << "\n";
		ofs << "enemySpawnJitterScale=" << gameplay.enemySpawnJitterScale << "\n";
		ofs << "enemySpawnMinPlayerDist=" << gameplay.enemySpawnMinPlayerDist << "\n";
		ofs << "enemySpawnMinEnemyDist=" << gameplay.enemySpawnMinEnemyDist << "\n";
		ofs << "pushSlop=" << gameplay.pushSlop << "\n";
		ofs << "playerPushShare=" << gameplay.playerPushShare << "\n";
		ofs << "enemyPushShare=" << gameplay.enemyPushShare << "\n";
		ofs << "difficultyPreset=" << gameplayDebug.difficultyPreset << "\n";
		ofs << "stageClearCount=" << roguelike.stageClearCount << "\n";
		ofs << "attackPowerLevel=" << roguelike.attackPowerLevel << "\n";
		ofs << "attackSpeedLevel=" << roguelike.attackSpeedLevel << "\n";
		ofs << "evadeCooldownLevel=" << roguelike.evadeCooldownLevel << "\n";
		ofs << "lastUpgradeType=" << roguelike.lastUpgradeType << "\n";
		ofs << "upgradeRerollMax=" << roguelike.rerollMaxPerStage << "\n";
		return ofs.good();
	};

	if (!IsDefaultPathArgument(path))
	{
		return writeToPath(ResolvePath(path));
	}

	bool anySaved = false;
	const char* syncPaths[] =
	{
		kDefaultGameplayTuningPath,
		kDebugGameplayTuningPath,
		kMirrorGameplayTuningPath,
		kUpstreamMirrorGameplayTuningPath
	};
	for (const char* syncPath : syncPaths)
	{
		if (writeToPath(syncPath))
		{
			anySaved = true;
		}
	}
	return anySaved;
}
