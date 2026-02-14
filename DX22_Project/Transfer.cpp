#include "Transfer.h"
#include <fstream>
#include <string>
#include <cstdlib>

namespace
{
	const char* kDefaultGameplayTuningPath = "Assets/gameplay_tuning.cfg";

	const char* ResolvePath(const char* path)
	{
		if (path && path[0] != '\0')
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
}

void Transfer::ResetGameplayTuningToDefault()
{
	gameplay = GameplayTuning{};
}

const char* Transfer::GetGameplayTuningPath() const
{
	return kDefaultGameplayTuningPath;
}

bool Transfer::LoadGameplayTuning(const char* path)
{
	const char* resolvedPath = ResolvePath(path);
	std::ifstream ifs(resolvedPath);
	if (!ifs.is_open())
	{
		return false;
	}

	GameplayTuning loaded{};
	int loadedPreset = gameplayDebug.difficultyPreset;

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
		else if (key == "attackWindup") loaded.attackWindup = ToFloat(value, loaded.attackWindup);
		else if (key == "attackDuration") loaded.attackDuration = ToFloat(value, loaded.attackDuration);
		else if (key == "attackRecovery") loaded.attackRecovery = ToFloat(value, loaded.attackRecovery);
		else if (key == "attackCooldown") loaded.attackCooldown = ToFloat(value, loaded.attackCooldown);
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
		else if (key == "playerDamageFlashScale") loaded.playerDamageFlashScale = ToFloat(value, loaded.playerDamageFlashScale);
		else if (key == "enemyDefeatFlash") loaded.enemyDefeatFlash = ToFloat(value, loaded.enemyDefeatFlash);
		else if (key == "enemyDefeatFlashScale") loaded.enemyDefeatFlashScale = ToFloat(value, loaded.enemyDefeatFlashScale);
		else if (key == "volumeMaster") loaded.volumeMaster = ToFloat(value, loaded.volumeMaster);
		else if (key == "volumeBgm") loaded.volumeBgm = ToFloat(value, loaded.volumeBgm);
		else if (key == "volumeSe") loaded.volumeSe = ToFloat(value, loaded.volumeSe);
		else if (key == "enemyAttackWindup") loaded.enemyAttackWindup = ToFloat(value, loaded.enemyAttackWindup);
		else if (key == "enemyAttackCooldown") loaded.enemyAttackCooldown = ToFloat(value, loaded.enemyAttackCooldown);
		else if (key == "enemyAttackRangeMin") loaded.enemyAttackRangeMin = ToFloat(value, loaded.enemyAttackRangeMin);
		else if (key == "enemyAttackRangeScale") loaded.enemyAttackRangeScale = ToFloat(value, loaded.enemyAttackRangeScale);
		else if (key == "enemyAttackDamage") loaded.enemyAttackDamage = ToFloat(value, loaded.enemyAttackDamage);
		else if (key == "enemyMoveSpeed") loaded.enemyMoveSpeed = ToFloat(value, loaded.enemyMoveSpeed);
		else if (key == "waveEnemyMoveSpeedAdd") loaded.waveEnemyMoveSpeedAdd = ToFloat(value, loaded.waveEnemyMoveSpeedAdd);
		else if (key == "waveEnemyAttackDamageScalePerWave") loaded.waveEnemyAttackDamageScalePerWave = ToFloat(value, loaded.waveEnemyAttackDamageScalePerWave);
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
	}

	gameplay = loaded;
	if (loadedPreset < 0) loadedPreset = 0;
	if (loadedPreset > 2) loadedPreset = 2;
	gameplayDebug.difficultyPreset = loadedPreset;
	return true;
}

bool Transfer::SaveGameplayTuning(const char* path) const
{
	const char* resolvedPath = ResolvePath(path);
	std::ofstream ofs(resolvedPath, std::ios::trunc);
	if (!ofs.is_open())
	{
		return false;
	}

	ofs << "# DX22 gameplay tuning\n";
	ofs << "enemyCount=" << gameplay.enemyCount << "\n";
	ofs << "waveMax=" << gameplay.waveMax << "\n";
	ofs << "waveEnemyAddPerWave=" << gameplay.waveEnemyAddPerWave << "\n";
	ofs << "attackWindup=" << gameplay.attackWindup << "\n";
	ofs << "attackDuration=" << gameplay.attackDuration << "\n";
	ofs << "attackRecovery=" << gameplay.attackRecovery << "\n";
	ofs << "attackCooldown=" << gameplay.attackCooldown << "\n";
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
	ofs << "playerDamageFlashScale=" << gameplay.playerDamageFlashScale << "\n";
	ofs << "enemyDefeatFlash=" << gameplay.enemyDefeatFlash << "\n";
	ofs << "enemyDefeatFlashScale=" << gameplay.enemyDefeatFlashScale << "\n";
	ofs << "volumeMaster=" << gameplay.volumeMaster << "\n";
	ofs << "volumeBgm=" << gameplay.volumeBgm << "\n";
	ofs << "volumeSe=" << gameplay.volumeSe << "\n";
	ofs << "enemyAttackWindup=" << gameplay.enemyAttackWindup << "\n";
	ofs << "enemyAttackCooldown=" << gameplay.enemyAttackCooldown << "\n";
	ofs << "enemyAttackRangeMin=" << gameplay.enemyAttackRangeMin << "\n";
	ofs << "enemyAttackRangeScale=" << gameplay.enemyAttackRangeScale << "\n";
	ofs << "enemyAttackDamage=" << gameplay.enemyAttackDamage << "\n";
	ofs << "enemyMoveSpeed=" << gameplay.enemyMoveSpeed << "\n";
	ofs << "waveEnemyMoveSpeedAdd=" << gameplay.waveEnemyMoveSpeedAdd << "\n";
	ofs << "waveEnemyAttackDamageScalePerWave=" << gameplay.waveEnemyAttackDamageScalePerWave << "\n";
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
	return ofs.good();
}
