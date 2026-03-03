#include "Transfer.h"
#include <fstream>
#include <string>
#include <cstdlib>

namespace
{
	// Default config paths used depending on launch location.
	const char* kDefaultGameplayTuningPath = "Assets/gameplay_tuning.cfg";
	const char* kMirrorGameplayTuningPath = "DX22_Project/Assets/gameplay_tuning.cfg";
	const char* kDebugGameplayTuningPath = "x64/Debug/Assets/gameplay_tuning.cfg";
	const char* kUpstreamMirrorGameplayTuningPath = "../../DX22_Project/Assets/gameplay_tuning.cfg";

	/**
	 * @brief 呼び出し側が既定パスを使いたい指定かどうかを判定します。
	 * @param path 呼び出し側が渡したパスです。
	 * @return nullptr または空文字なら true です。
	 */
	bool IsDefaultPathArgument(const char* path)
	{
		return !(path && path[0] != '\0');
	}

	/**
	 * @brief 実際に使う設定ファイルパスを決定します。
	 * @param path 呼び出し側指定パスです。
	 * @return 指定があればそのパス、無ければ既定パスです。
	 */
	const char* ResolvePath(const char* path)
	{
		// 明示指定がある場合は、そのパスを優先します。
		if (!IsDefaultPathArgument(path))
		{
			return path;
		}
		// 未指定時だけ既定パスを返します。
		return kDefaultGameplayTuningPath;
	}

	/**
	 * @brief 文字列前後の空白を除去します。
	 * @param s 前後空白を除去する文字列です。
	 */
	void Trim(std::string& s)
	{
		size_t begin = 0;
		// 先頭側の空白を読み飛ばします。
		while (begin < s.size() && (s[begin] == ' ' || s[begin] == '\t' || s[begin] == '\r' || s[begin] == '\n'))
		{
			++begin;
		}

		size_t end = s.size();
		// 末尾側の空白も同様に削ります。
		while (end > begin && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n'))
		{
			--end;
		}

		// 空白を除いた範囲だけ残します。
		s = s.substr(begin, end - begin);
	}

	/**
	 * @brief 文字列を整数へ変換します。
	 * @param s 変換元文字列です。
	 * @param fallback 変換できない場合に返す値です。
	 * @return 変換成功時は整数値、失敗時は fallback です。
	 */
	int ToInt(const std::string& s, int fallback)
	{
		char* endPtr = nullptr;
		const long v = std::strtol(s.c_str(), &endPtr, 10);
		// 数字として 1 文字も読めなかった場合は既定値へ戻します。
		if (endPtr == s.c_str())
		{
			return fallback;
		}
		return static_cast<int>(v);
	}

	/**
	 * @brief 文字列を浮動小数へ変換します。
	 * @param s 変換元文字列です。
	 * @param fallback 変換できない場合に返す値です。
	 * @return 変換成功時は浮動小数値、失敗時は fallback です。
	 */
	float ToFloat(const std::string& s, float fallback)
	{
		char* endPtr = nullptr;
		const float v = std::strtof(s.c_str(), &endPtr);
		// 数値解釈できない場合は既定値を維持します。
		if (endPtr == s.c_str())
		{
			return fallback;
		}
		return v;
	}

	/**
	 * @brief 整数値を指定範囲に丸めます。
	 * @param v 補正対象値です。
	 * @param lo 下限です。
	 * @param hi 上限です。
	 * @return lo 以上 hi 以下に丸めた値です。
	 */
	int ClampInt(int v, int lo, int hi)
	{
		if (v < lo) return lo;
		if (v > hi) return hi;
		return v;
	}

	/**
	 * @brief 難易度プリセット値を有効範囲へ補正します。
	 * @param preset 補正対象値です。
	 * @return 0 から 2 の範囲に収めた値です。
	 */
	int NormalizeDifficultyPresetValue(int preset)
	{
		return ClampInt(preset, 0, 2);
	}

	/**
	 * @brief 難易度に応じた小/大強化幅を返します。
	 * @param preset 難易度です。
	 * @param smallStep 小強化の増加量出力先です。
	 * @param largeStep 大強化の増加量出力先です。
	 */
	void GetUpgradeStepsForDifficulty(int preset, int& smallStep, int& largeStep)
	{
		// 難易度が高いほど、1回あたりの強化量を大きくします。
		switch (NormalizeDifficultyPresetValue(preset))
		{
		case 0:
			smallStep = 1;
			largeStep = 2;
			break;
		case 2:
			smallStep = 3;
			largeStep = 5;
			break;
		default:
			smallStep = 2;
			largeStep = 3;
			break;
		}
	}

	const int kUpgradeTierMax = 10;

	/**
	 * @brief 強化レベルを有効範囲へ丸めます。
	 * @param level 補正対象レベルです。
	 * @return 0 から上限までに丸めたレベルです。
	 */
	int ClampUpgradeTier(int level)
	{
		return ClampInt(level, 0, kUpgradeTierMax);
	}

	// Level tables used to convert upgrade tiers into actual runtime values.
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

	/**
	 * @brief 指定強化タイプがまだ提示可能かどうかを返します。
	 * @param upgradeType 判定する強化タイプです。
	 * @param attackPowerLevel 現在の攻撃力レベルです。
	 * @param attackSpeedLevel 現在の攻撃速度レベルです。
	 * @param evadeCooldownLevel 現在の回避短縮レベルです。
	 * @return 上限未到達なら true です。
	 */
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

	/**
	 * @brief 現在レベルに応じて提示可能な強化候補を生成します。
	 * @param offers 生成した候補の書き込み先です。
	 * @param attackPowerLevel 現在の攻撃力レベルです。
	 * @param attackSpeedLevel 現在の攻撃速度レベルです。
	 * @param evadeCooldownLevel 現在の回避短縮レベルです。
	 */
	void GenerateUpgradeOffers(int offers[kUpgradeOfferCount], int attackPowerLevel, int attackSpeedLevel, int evadeCooldownLevel)
	{
		int available[kUpgradeTypeCount]{};
		int availableCount = 0;
		for (int t = 0; t < kUpgradeTypeCount; ++t)
		{
			// 上限に達していない候補だけ抽出します。
			if (IsUpgradeTypeAvailable(t, attackPowerLevel, attackSpeedLevel, evadeCooldownLevel))
			{
				available[availableCount++] = t;
			}
		}

		if (availableCount == 0)
		{
			// 提示可能な強化が無い場合は全スロットを空にします。
			for (int i = 0; i < kUpgradeOfferCount; ++i)
			{
				offers[i] = kUpgradeOfferNone;
			}
			return;
		}

		// Fisher-Yates 風に並びを崩し、提示順を毎回変えます。
		for (int i = 0; i < availableCount; ++i)
		{
			const int j = RandRangeInt(i, availableCount - 1);
			const int tmp = available[i];
			available[i] = available[j];
			available[j] = tmp;
		}

		// 候補数が足りない場合は、利用可能候補から重複許可で埋めます。
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

	/**
	 * @brief 強化タイプに応じて対象レベルを増やします。
	 * @param attackPowerLevel 攻撃力レベル参照です。
	 * @param attackSpeedLevel 攻撃速度レベル参照です。
	 * @param evadeCooldownLevel 回避短縮レベル参照です。
	 * @param upgradeType 適用する強化タイプです。
	 * @param smallStep 小強化量です。
	 * @param largeStep 大強化量です。
	 */
	void ApplyUpgradeType(int& attackPowerLevel,
						  int& attackSpeedLevel,
						  int& evadeCooldownLevel,
						  int upgradeType,
						  int smallStep,
						  int largeStep)
	{
		switch (upgradeType)
		{
		case 0:
			attackPowerLevel = ClampUpgradeTier(attackPowerLevel + smallStep);
			break;
		case 1:
			attackSpeedLevel = ClampUpgradeTier(attackSpeedLevel + smallStep);
			break;
		case 2:
			evadeCooldownLevel = ClampUpgradeTier(evadeCooldownLevel + smallStep);
			break;
		case 3:
			attackPowerLevel = ClampUpgradeTier(attackPowerLevel + largeStep);
			break;
		case 4:
			attackSpeedLevel = ClampUpgradeTier(attackSpeedLevel + largeStep);
			break;
		case 5:
			evadeCooldownLevel = ClampUpgradeTier(evadeCooldownLevel + largeStep);
			break;
		default:
			break;
		}
	}
}

/**
 * @brief ゲームプレイ調整値を既定値へ戻します。
 */
void Transfer::ResetGameplayTuningToDefault()
{
	// 既定構築子で丸ごと初期値へ戻します。
	gameplay = GameplayTuning{};
}

/**
 * @brief ローグライク強化状態を初期値へ戻します。
 */
void Transfer::ResetRoguelikeUpgrade()
{
	// 選択候補や残リロールも含めて、すべて初期化します。
	roguelike = RoguelikeUpgrade{};
}

/**
 * @brief 難易度プリセットに応じた基準値を反映します。
 * @param preset 0:Easy 1:Normal 2:Hard の難易度値です。
 */
void Transfer::ApplyDifficultyPreset(int preset)
{
	const int normalizedPreset = NormalizeDifficultyPresetValue(preset);
	// 難易度ごとに敵数、Wave 数、敵性能の基準値を切り替えます。
	switch (normalizedPreset)
	{
	case 0: // Easy
		gameplay.enemyCount = 2;
		gameplay.waveMax = 2;
		gameplay.waveEnemyAddPerWave = 1;
		gameplay.enemyAttackWindup = 0.65f;
		gameplay.enemyAttackCooldown = 1.20f;
		gameplay.enemyAttackRangeMin = 0.70f;
		gameplay.enemyAttackRangeScale = 1.20f;
		gameplay.enemyAttackDamage = 0.8f;
		gameplay.enemyMoveSpeed = 1.00f;
		gameplay.waveEnemyMoveSpeedAdd = 0.08f;
		gameplay.waveEnemyAttackDamageScalePerWave = 0.10f;
		break;
	case 2: // Hard
		gameplay.enemyCount = 4;
		gameplay.waveMax = 4;
		gameplay.waveEnemyAddPerWave = 2;
		gameplay.enemyAttackWindup = 0.45f;
		gameplay.enemyAttackCooldown = 0.80f;
		gameplay.enemyAttackRangeMin = 0.90f;
		gameplay.enemyAttackRangeScale = 1.45f;
		gameplay.enemyAttackDamage = 1.3f;
		gameplay.enemyMoveSpeed = 1.35f;
		gameplay.waveEnemyMoveSpeedAdd = 0.22f;
		gameplay.waveEnemyAttackDamageScalePerWave = 0.32f;
		break;
	default: // Normal
		gameplay.enemyCount = 3;
		gameplay.waveMax = 3;
		gameplay.waveEnemyAddPerWave = 1;
		gameplay.enemyAttackWindup = 0.55f;
		gameplay.enemyAttackCooldown = 1.00f;
		gameplay.enemyAttackRangeMin = 0.80f;
		gameplay.enemyAttackRangeScale = 1.35f;
		gameplay.enemyAttackDamage = 1.0f;
		gameplay.enemyMoveSpeed = 1.20f;
		gameplay.waveEnemyMoveSpeedAdd = 0.15f;
		gameplay.waveEnemyAttackDamageScalePerWave = 0.20f;
		break;
	}

	// 実際に適用した難易度はデバッグ表示とタイトル UI にも反映します。
	gameplayDebug.difficultyPreset = normalizedPreset;
	gameplayDebug.titleDifficultySelection = normalizedPreset;
}

/**
 * @brief ステージクリア時の自動強化を適用します。
 */
void Transfer::ApplyStageClearUpgrade()
{
	int smallStep = 1;
	int largeStep = 2;
	GetUpgradeStepsForDifficulty(gameplayDebug.difficultyPreset, smallStep, largeStep);

	// 旧自動付与ルートでは 3 種を順番に回して、強化の偏りを抑えます。
	const int nextType = roguelike.stageClearCount % 3;
	++roguelike.stageClearCount;
	roguelike.lastUpgradeType = nextType;
	ApplyUpgradeType(
		roguelike.attackPowerLevel,
		roguelike.attackSpeedLevel,
		roguelike.evadeCooldownLevel,
		nextType,
		smallStep,
		largeStep);
}

/**
 * @brief 三択強化候補の提示を開始します。
 */
void Transfer::BeginUpgradeSelection()
{
	// 選択待ちへ入り、現在の上限設定からリロール残数を初期化します。
	roguelike.selectionPending = 1;
	roguelike.rerollRemain = ClampInt(roguelike.rerollMaxPerStage, 0, 99);
	GenerateUpgradeOffers(roguelike.offers, roguelike.attackPowerLevel, roguelike.attackSpeedLevel, roguelike.evadeCooldownLevel);
}

/**
 * @brief 現在の強化候補を再抽選します。
 * @return 再抽選できた場合は true です。
 */
bool Transfer::RerollUpgradeSelection()
{
	// 選択待ちでなければ、そもそも再抽選する候補がありません。
	if (roguelike.selectionPending == 0) return false;
	// 残回数が無い場合も再抽選できません。
	if (roguelike.rerollRemain <= 0) return false;
	--roguelike.rerollRemain;
	GenerateUpgradeOffers(roguelike.offers, roguelike.attackPowerLevel, roguelike.attackSpeedLevel, roguelike.evadeCooldownLevel);
	return true;
}

/**
 * @brief 指定した候補番号の強化を適用します。
 * @param offerIndex 選択した候補の添字です。
 * @return 適用に成功した場合は true です。
 */
bool Transfer::ApplyUpgradeSelection(int offerIndex)
{
	// 選択待ちでない状態では適用しません。
	if (roguelike.selectionPending == 0) return false;
	// 候補範囲外の番号は無効です。
	if (offerIndex < 0 || offerIndex >= RoguelikeUpgrade::kOfferCount) return false;

	const int selectedType = roguelike.offers[offerIndex];
	// 候補が空か壊れている場合は適用しません。
	if (selectedType < 0 || selectedType >= RoguelikeUpgrade::UpgradeTypeCount) return false;
	int smallStep = 1;
	int largeStep = 2;
	GetUpgradeStepsForDifficulty(gameplayDebug.difficultyPreset, smallStep, largeStep);
	ApplyUpgradeType(
		roguelike.attackPowerLevel,
		roguelike.attackSpeedLevel,
		roguelike.evadeCooldownLevel,
		selectedType,
		smallStep,
		largeStep);
	++roguelike.stageClearCount;
	roguelike.lastUpgradeType = selectedType;
	roguelike.selectionPending = 0;
	roguelike.rerollRemain = 0;
	return true;
}

/**
 * @brief 難易度値を有効範囲へ丸めます。
 * @param preset 補正対象の難易度値です。
 * @return 0 から 2 に丸めた値です。
 */
int Transfer::NormalizeDifficultyPreset(int preset) const
{
	return NormalizeDifficultyPresetValue(preset);
}

/**
 * @brief 強化レベルを有効範囲へ丸めます。
 * @param level 補正対象レベルです。
 * @return 0 から上限までに丸めた値です。
 */
int Transfer::ClampUpgradeLevel(int level) const
{
	return ClampUpgradeTier(level);
}

/**
 * @brief 強化レベル上限を返します。
 * @return 強化レベル上限です。
 */
int Transfer::GetUpgradeLevelMax() const
{
	return RoguelikeUpgrade::kLevelMax;
}

/**
 * @brief 強化タイプと難易度から実際の増加量を返します。
 * @param upgradeType 強化種別です。
 * @param difficultyPreset 難易度です。
 * @return その強化で増えるレベル数です。
 */
int Transfer::GetUpgradeStepForType(int upgradeType, int difficultyPreset) const
{
	int smallStep = 1;
	int largeStep = 2;
	GetUpgradeStepsForDifficulty(difficultyPreset, smallStep, largeStep);

	// Small / Large の別に応じて、難易度別の増加量を返します。
	switch (upgradeType)
	{
	case RoguelikeUpgrade::UpgradeAttackPower:
	case RoguelikeUpgrade::UpgradeAttackSpeed:
	case RoguelikeUpgrade::UpgradeEvadeCooldown:
		return smallStep;
	case RoguelikeUpgrade::UpgradeAttackPowerLarge:
	case RoguelikeUpgrade::UpgradeAttackSpeedLarge:
	case RoguelikeUpgrade::UpgradeEvadeCooldownLarge:
		return largeStep;
	default:
		return 0;
	}
}

/**
 * @brief 3 系統の強化レベル合計を返します。
 * @return 合計強化レベルです。
 */
int Transfer::GetTotalUpgradeLevels() const
{
	return
		ClampUpgradeTier(roguelike.attackPowerLevel) +
		ClampUpgradeTier(roguelike.attackSpeedLevel) +
		ClampUpgradeTier(roguelike.evadeCooldownLevel);
}

/**
 * @brief 攻撃力レベルから実ダメージ値を返します。
 * @param level 攻撃力レベルです。
 * @return 実ダメージ値です。
 */
int Transfer::GetPlayerAttackDamageByLevel(int level) const
{
	return kAttackDamageByTier[ClampUpgradeTier(level)];
}

/**
 * @brief 攻撃速度レベルから攻撃クールタイム倍率を返します。
 * @param level 攻撃速度レベルです。
 * @return クールタイム倍率です。
 */
float Transfer::GetAttackCooldownScaleByLevel(int level) const
{
	return kAttackCooldownScaleByTier[ClampUpgradeTier(level)];
}

/**
 * @brief 回避強化レベルから回避クールタイム倍率を返します。
 * @param level 回避強化レベルです。
 * @return クールタイム倍率です。
 */
float Transfer::GetEvadeCooldownScaleByLevel(int level) const
{
	return kEvadeCooldownScaleByTier[ClampUpgradeTier(level)];
}

/**
 * @brief 強化進行度に応じた通常敵 HP 倍率を返します。
 * @return 敵 HP 倍率です。
 */
float Transfer::GetEnemyHpScaleByUpgradeProgress() const
{
	// 合計強化 10 ごとに 1 段階だけ上げ、極端な伸びを抑えます。
	const int progressTier = ClampInt(GetTotalUpgradeLevels() / 10, 0, 3);
	return 1.0f + 0.20f * static_cast<float>(progressTier);
}

/**
 * @brief 強化進行度に応じた通常敵攻撃倍率を返します。
 * @return 敵攻撃倍率です。
 */
float Transfer::GetEnemyAttackScaleByUpgradeProgress() const
{
	const int progressTier = ClampInt(GetTotalUpgradeLevels() / 10, 0, 3);
	return 1.0f + 0.10f * static_cast<float>(progressTier);
}

/**
 * @brief 難易度に応じたボス HP 倍率を返します。
 * @param preset 難易度です。
 * @return ボス HP 倍率です。
 */
float Transfer::GetBossHpScaleByDifficulty(int preset) const
{
	switch (NormalizeDifficultyPresetValue(preset))
	{
	case 0: return 0.85f;
	case 2: return 1.25f;
	default: return 1.0f;
	}
}

/**
 * @brief 難易度に応じたボス行動頻度倍率を返します。
 * @param preset 難易度です。
 * @return ボスのクールタイム倍率です。
 */
float Transfer::GetBossCooldownScaleByDifficulty(int preset) const
{
	switch (NormalizeDifficultyPresetValue(preset))
	{
	case 0: return 1.15f;
	case 2: return 0.85f;
	default: return 1.0f;
	}
}


/**
 * @brief 既定のゲームプレイ設定ファイルパスを返します。
 * @return 既定設定ファイルパスです。
 */
const char* Transfer::GetGameplayTuningPath() const
{
	return kDefaultGameplayTuningPath;
}

/**
 * @brief 設定ファイルからゲームプレイ調整値を読み込みます。
 * @param path 読み込むファイルパスです。nullptr の場合は既定パスです。
 * @return 読み込みに成功した場合は true です。
 */
bool Transfer::LoadGameplayTuning(const char* path)
{
	const char* resolvedPath = ResolvePath(path);
	std::ifstream ifs(resolvedPath);

	// 既定パスで開けない場合は、実行場所ごとの代表パスも順に試します。
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

	// 読み込み途中で失敗しても既存値を壊さないよう、一旦ローカルへ集めます。
	GameplayTuning loaded{};
	int loadedPreset = gameplayDebug.difficultyPreset;
	RoguelikeUpgrade loadedRogue = roguelike;

	std::string line;
	while (std::getline(ifs, line))
	{
		// 空行とコメント行は設定値ではないので無視します。
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

		// キー名に応じて、対象項目だけを個別に復元します。
		if (key == "enemyCount") loaded.enemyCount = ToInt(value, loaded.enemyCount);
		else if (key == "waveMax") loaded.waveMax = ToInt(value, loaded.waveMax);
		else if (key == "waveEnemyAddPerWave") loaded.waveEnemyAddPerWave = ToInt(value, loaded.waveEnemyAddPerWave);
		else if (key == "groundTileSize") loaded.groundTileSize = ToFloat(value, loaded.groundTileSize);
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
		else if (key == "bossGuardBarOffsetX") loaded.bossGuardBarOffsetX = ToFloat(value, loaded.bossGuardBarOffsetX);
		else if (key == "bossGuardBarOffsetY") loaded.bossGuardBarOffsetY = ToFloat(value, loaded.bossGuardBarOffsetY);
		else if (key == "bossGuardBarWidthRate") loaded.bossGuardBarWidthRate = ToFloat(value, loaded.bossGuardBarWidthRate);
		else if (key == "bossGuardBarHeightRate") loaded.bossGuardBarHeightRate = ToFloat(value, loaded.bossGuardBarHeightRate);
		else if (key == "bossSizeAreaScale") loaded.bossSizeAreaScale = ToFloat(value, loaded.bossSizeAreaScale);
		else if (key == "bossMaxHp") loaded.bossMaxHp = ToInt(value, loaded.bossMaxHp);
		else if (key == "bossAttackTelegraph") loaded.bossAttackTelegraph = ToFloat(value, loaded.bossAttackTelegraph);
		else if (key == "bossAttackJumpOutTime") loaded.bossAttackJumpOutTime = ToFloat(value, loaded.bossAttackJumpOutTime);
		else if (key == "bossAttackDashDuration") loaded.bossAttackDashDuration = ToFloat(value, loaded.bossAttackDashDuration);
		else if (key == "bossAttackCooldown") loaded.bossAttackCooldown = ToFloat(value, loaded.bossAttackCooldown);
		else if (key == "bossAttackLanePlayerScale") loaded.bossAttackLanePlayerScale = ToFloat(value, loaded.bossAttackLanePlayerScale);
		else if (key == "bossAttackDamage") loaded.bossAttackDamage = ToFloat(value, loaded.bossAttackDamage);
		else if (key == "bossGuardInitialMax") loaded.bossGuardInitialMax = ToFloat(value, loaded.bossGuardInitialMax);
		else if (key == "bossGuardFinalMax") loaded.bossGuardFinalMax = ToFloat(value, loaded.bossGuardFinalMax);
		else if (key == "bossGuardRecoverStep") loaded.bossGuardRecoverStep = ToFloat(value, loaded.bossGuardRecoverStep);
		else if (key == "bossDamageScaleNormal") loaded.bossDamageScaleNormal = ToFloat(value, loaded.bossDamageScaleNormal);
		else if (key == "bossDamageScaleBroken") loaded.bossDamageScaleBroken = ToFloat(value, loaded.bossDamageScaleBroken);
		else if (key == "bossBreakRecoverSec") loaded.bossBreakRecoverSec = ToFloat(value, loaded.bossBreakRecoverSec);
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
		else if (key == "bossUltimateStompRepeatTelegraph") loaded.bossUltimateStompRepeatTelegraph = ToFloat(value, loaded.bossUltimateStompRepeatTelegraph);
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

	// 読み込んだ値はここで全体的にクランプし、壊れた設定を防ぎます。
	loaded.screenShakeHitThreshold = ClampInt(loaded.screenShakeHitThreshold, 1, 16);
	if (loaded.groundTileSize < 0.5f) loaded.groundTileSize = 0.5f;
	if (loaded.groundTileSize > 10.0f) loaded.groundTileSize = 10.0f;
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
	if (loaded.bossGuardBarOffsetX < -960.0f) loaded.bossGuardBarOffsetX = -960.0f;
	if (loaded.bossGuardBarOffsetX > 960.0f) loaded.bossGuardBarOffsetX = 960.0f;
	if (loaded.bossGuardBarOffsetY < -120.0f) loaded.bossGuardBarOffsetY = -120.0f;
	if (loaded.bossGuardBarOffsetY > 320.0f) loaded.bossGuardBarOffsetY = 320.0f;
	if (loaded.bossGuardBarWidthRate < 0.10f) loaded.bossGuardBarWidthRate = 0.10f;
	if (loaded.bossGuardBarWidthRate > 0.90f) loaded.bossGuardBarWidthRate = 0.90f;
	if (loaded.bossGuardBarHeightRate < 0.005f) loaded.bossGuardBarHeightRate = 0.005f;
	if (loaded.bossGuardBarHeightRate > 0.10f) loaded.bossGuardBarHeightRate = 0.10f;
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
	if (loaded.bossGuardInitialMax < 1.0f) loaded.bossGuardInitialMax = 1.0f;
	if (loaded.bossGuardInitialMax > 200.0f) loaded.bossGuardInitialMax = 200.0f;
	if (loaded.bossGuardFinalMax < 1.0f) loaded.bossGuardFinalMax = 1.0f;
	if (loaded.bossGuardFinalMax > 200.0f) loaded.bossGuardFinalMax = 200.0f;
	if (loaded.bossGuardFinalMax < loaded.bossGuardInitialMax) loaded.bossGuardFinalMax = loaded.bossGuardInitialMax;
	if (loaded.bossGuardRecoverStep < 0.0f) loaded.bossGuardRecoverStep = 0.0f;
	if (loaded.bossGuardRecoverStep > 50.0f) loaded.bossGuardRecoverStep = 50.0f;
	if (loaded.bossDamageScaleNormal < 0.0f) loaded.bossDamageScaleNormal = 0.0f;
	if (loaded.bossDamageScaleNormal > 5.0f) loaded.bossDamageScaleNormal = 5.0f;
	if (loaded.bossDamageScaleBroken < 0.0f) loaded.bossDamageScaleBroken = 0.0f;
	if (loaded.bossDamageScaleBroken > 10.0f) loaded.bossDamageScaleBroken = 10.0f;
	if (loaded.bossBreakRecoverSec < 1.0f) loaded.bossBreakRecoverSec = 1.0f;
	if (loaded.bossBreakRecoverSec > 30.0f) loaded.bossBreakRecoverSec = 30.0f;
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
	if (loaded.bossUltimateStompRepeatTelegraph < 0.10f) loaded.bossUltimateStompRepeatTelegraph = 0.10f;
	if (loaded.bossUltimateStompRadiusScale < 0.5f) loaded.bossUltimateStompRadiusScale = 0.5f;
	if (loaded.bossUltimateFieldTelegraph < 0.10f) loaded.bossUltimateFieldTelegraph = 0.10f;
	if (loaded.bossUltimateFieldSafeScale < 0.5f) loaded.bossUltimateFieldSafeScale = 0.5f;

	// 正常化済みの値だけを本体へ反映します。
	gameplay = loaded;
	loadedPreset = ClampInt(loadedPreset, 0, 2);
	gameplayDebug.difficultyPreset = loadedPreset;
	gameplayDebug.titleDifficultySelection = loadedPreset;
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

/**
 * @brief 現在のゲームプレイ調整値を設定ファイルへ保存します。
 * @param path 保存先パスです。nullptr の場合は既定同期パス群へ保存します。
 * @return 1 つ以上保存に成功した場合は true です。
 */
bool Transfer::SaveGameplayTuning(const char* path) const
{
	// 単一パスへ書き出すためのローカル関数です。
	auto writeToPath = [&](const char* targetPath) -> bool
	{
		std::ofstream ofs(targetPath, std::ios::trunc);
		if (!ofs.is_open())
		{
			return false;
		}

		// すべての調整値を key=value 形式で書き出し、次回起動時に復元できるようにします。
		ofs << "# DX22 gameplay tuning\n";
		ofs << "enemyCount=" << gameplay.enemyCount << "\n";
		ofs << "waveMax=" << gameplay.waveMax << "\n";
		ofs << "waveEnemyAddPerWave=" << gameplay.waveEnemyAddPerWave << "\n";
		ofs << "groundTileSize=" << gameplay.groundTileSize << "\n";
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
		ofs << "bossGuardBarOffsetX=" << gameplay.bossGuardBarOffsetX << "\n";
		ofs << "bossGuardBarOffsetY=" << gameplay.bossGuardBarOffsetY << "\n";
		ofs << "bossGuardBarWidthRate=" << gameplay.bossGuardBarWidthRate << "\n";
		ofs << "bossGuardBarHeightRate=" << gameplay.bossGuardBarHeightRate << "\n";
		ofs << "bossSizeAreaScale=" << gameplay.bossSizeAreaScale << "\n";
		ofs << "bossMaxHp=" << gameplay.bossMaxHp << "\n";
		ofs << "bossAttackTelegraph=" << gameplay.bossAttackTelegraph << "\n";
		ofs << "bossAttackJumpOutTime=" << gameplay.bossAttackJumpOutTime << "\n";
		ofs << "bossAttackDashDuration=" << gameplay.bossAttackDashDuration << "\n";
		ofs << "bossAttackCooldown=" << gameplay.bossAttackCooldown << "\n";
		ofs << "bossAttackLanePlayerScale=" << gameplay.bossAttackLanePlayerScale << "\n";
		ofs << "bossAttackDamage=" << gameplay.bossAttackDamage << "\n";
		ofs << "bossGuardInitialMax=" << gameplay.bossGuardInitialMax << "\n";
		ofs << "bossGuardFinalMax=" << gameplay.bossGuardFinalMax << "\n";
		ofs << "bossGuardRecoverStep=" << gameplay.bossGuardRecoverStep << "\n";
		ofs << "bossDamageScaleNormal=" << gameplay.bossDamageScaleNormal << "\n";
		ofs << "bossDamageScaleBroken=" << gameplay.bossDamageScaleBroken << "\n";
		ofs << "bossBreakRecoverSec=" << gameplay.bossBreakRecoverSec << "\n";
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
		ofs << "bossUltimateStompRepeatTelegraph=" << gameplay.bossUltimateStompRepeatTelegraph << "\n";
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

	// 明示パス指定時は、そのパスだけを更新します。
	if (!IsDefaultPathArgument(path))
	{
		return writeToPath(ResolvePath(path));
	}

	bool anySaved = false;
	// 既定保存時は実行側/ソース側の代表パスへ同期保存します。
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
