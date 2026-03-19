#include "BossAttackScript.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace
{
	const char* kDefaultBossAttackScriptPath = "Assets/boss_attack_profiles.json";
	const char* kMirrorBossAttackScriptPath = "DX22_Project/Assets/boss_attack_profiles.json";
	const char* kDebugBossAttackScriptPath = "x64/Debug/Assets/boss_attack_profiles.json";
	const char* kUpstreamMirrorBossAttackScriptPath = "../../DX22_Project/Assets/boss_attack_profiles.json";
	const int kCurrentVersion = 4;

	void Trim(std::string& text)
	{
		const size_t begin = text.find_first_not_of(" \t\r\n");
		if (begin == std::string::npos)
		{
			text.clear();
			return;
		}
		const size_t end = text.find_last_not_of(" \t\r\n");
		text = text.substr(begin, end - begin + 1);
	}

	bool IsDefaultPathArgument(const char* path)
	{
		return (path == nullptr) || (path[0] == '\0');
	}

	const char* ResolvePath(const char* path)
	{
		return IsDefaultPathArgument(path) ? kDefaultBossAttackScriptPath : path;
	}

	float ClampFloat(float value, float minValue, float maxValue)
	{
		if (value < minValue) return minValue;
		if (value > maxValue) return maxValue;
		return value;
	}

	int ClampIntValue(int value, int minValue, int maxValue)
	{
		if (value < minValue) return minValue;
		if (value > maxValue) return maxValue;
		return value;
	}

	bool TryParseInt(const std::string& text, int& outValue)
	{
		try
		{
			outValue = std::stoi(text);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	bool TryParseFloat(const std::string& text, float& outValue)
	{
		try
		{
			outValue = std::stof(text);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	bool TryParseBool(const std::string& text, bool& outValue)
	{
		if (text == "true" || text == "True" || text == "1")
		{
			outValue = true;
			return true;
		}
		if (text == "false" || text == "False" || text == "0")
		{
			outValue = false;
			return true;
		}
		return false;
	}

	size_t FindValueStart(const std::string& text, const std::string& keyToken)
	{
		const size_t keyPos = text.find(keyToken);
		if (keyPos == std::string::npos)
		{
			return std::string::npos;
		}
		const size_t colonPos = text.find(':', keyPos + keyToken.size());
		if (colonPos == std::string::npos)
		{
			return std::string::npos;
		}
		return text.find_first_not_of(" \t\r\n", colonPos + 1);
	}

	bool ExtractQuotedString(const std::string& text, size_t startPos, std::string& outValue)
	{
		if (startPos == std::string::npos || startPos >= text.size() || text[startPos] != '"')
		{
			return false;
		}

		std::string result;
		for (size_t i = startPos + 1; i < text.size(); ++i)
		{
			const char c = text[i];
			if (c == '\\')
			{
				if (i + 1 < text.size())
				{
					result.push_back(text[i + 1]);
					++i;
				}
				continue;
			}
			if (c == '"')
			{
				outValue = result;
				return true;
			}
			result.push_back(c);
		}
		return false;
	}

	bool ExtractTokenValue(const std::string& text, size_t startPos, std::string& outValue)
	{
		if (startPos == std::string::npos || startPos >= text.size())
		{
			return false;
		}
		size_t endPos = startPos;
		while (endPos < text.size())
		{
			const char c = text[endPos];
			if (c == ',' || c == '}' || c == ']' || c == '\r' || c == '\n')
			{
				break;
			}
			++endPos;
		}
		outValue = text.substr(startPos, endPos - startPos);
		Trim(outValue);
		return !outValue.empty();
	}

	bool ExtractEnclosedValue(const std::string& text, size_t startPos, char openChar, char closeChar, std::string& outValue)
	{
		if (startPos == std::string::npos || startPos >= text.size() || text[startPos] != openChar)
		{
			return false;
		}

		int depth = 0;
		bool inString = false;
		for (size_t i = startPos; i < text.size(); ++i)
		{
			const char c = text[i];
			if (c == '"' && (i == 0 || text[i - 1] != '\\'))
			{
				inString = !inString;
			}
			if (inString)
			{
				continue;
			}
			if (c == openChar)
			{
				++depth;
			}
			else if (c == closeChar)
			{
				--depth;
				if (depth == 0)
				{
					outValue = text.substr(startPos, i - startPos + 1);
					return true;
				}
			}
		}
		return false;
	}

	bool TryGetStringValue(const std::string& text, const char* key, std::string& outValue)
	{
		const size_t startPos = FindValueStart(text, std::string("\"") + key + "\"");
		return ExtractQuotedString(text, startPos, outValue);
	}

	bool TryGetFloatValue(const std::string& text, const char* key, float& outValue)
	{
		const size_t startPos = FindValueStart(text, std::string("\"") + key + "\"");
		std::string token;
		return ExtractTokenValue(text, startPos, token) && TryParseFloat(token, outValue);
	}

	bool TryGetIntValue(const std::string& text, const char* key, int& outValue)
	{
		const size_t startPos = FindValueStart(text, std::string("\"") + key + "\"");
		std::string token;
		return ExtractTokenValue(text, startPos, token) && TryParseInt(token, outValue);
	}

	bool TryGetBoolValue(const std::string& text, const char* key, bool& outValue)
	{
		const size_t startPos = FindValueStart(text, std::string("\"") + key + "\"");
		std::string token;
		return ExtractTokenValue(text, startPos, token) && TryParseBool(token, outValue);
	}

	bool TryGetObjectValue(const std::string& text, const char* key, std::string& outValue)
	{
		const size_t startPos = FindValueStart(text, std::string("\"") + key + "\"");
		return ExtractEnclosedValue(text, startPos, '{', '}', outValue);
	}

	bool TryGetArrayValue(const std::string& text, const char* key, std::string& outValue)
	{
		const size_t startPos = FindValueStart(text, std::string("\"") + key + "\"");
		return ExtractEnclosedValue(text, startPos, '[', ']', outValue);
	}

	std::vector<std::string> SplitTopLevelObjects(const std::string& arrayText)
	{
		std::vector<std::string> objects;
		bool inString = false;
		int depth = 0;
		size_t objectStart = std::string::npos;
		for (size_t i = 0; i < arrayText.size(); ++i)
		{
			const char c = arrayText[i];
			if (c == '"' && (i == 0 || arrayText[i - 1] != '\\'))
			{
				inString = !inString;
			}
			if (inString)
			{
				continue;
			}
			if (c == '{')
			{
				if (depth == 0)
				{
					objectStart = i;
				}
				++depth;
			}
			else if (c == '}')
			{
				--depth;
				if (depth == 0 && objectStart != std::string::npos)
				{
					objects.push_back(arrayText.substr(objectStart, i - objectStart + 1));
					objectStart = std::string::npos;
				}
			}
		}
		return objects;
	}

	std::vector<std::string> SplitTopLevelTokens(const std::string& arrayText)
	{
		std::vector<std::string> tokens;
		std::string current;
		int depth = 0;
		bool inString = false;
		for (size_t i = 0; i < arrayText.size(); ++i)
		{
			const char c = arrayText[i];
			if (c == '"' && (i == 0 || arrayText[i - 1] != '\\'))
			{
				inString = !inString;
			}
			if (!inString)
			{
				if (c == '[' || c == '{')
				{
					++depth;
				}
				else if (c == ']' || c == '}')
				{
					--depth;
				}
				else if (c == ',' && depth == 1)
				{
					Trim(current);
					if (!current.empty())
					{
						tokens.push_back(current);
					}
					current.clear();
					continue;
				}
			}
			if (depth >= 1 && !(depth == 1 && (c == '[' || c == ']')))
			{
				current.push_back(c);
			}
		}
		Trim(current);
		if (!current.empty())
		{
			tokens.push_back(current);
		}
		return tokens;
	}

	std::string EscapeJson(const std::string& text)
	{
		std::string result;
		result.reserve(text.size() + 8);
		for (char c : text)
		{
			switch (c)
			{
			case '\\': result += "\\\\"; break;
			case '"': result += "\\\""; break;
			case '\n': result += "\\n"; break;
			case '\r': result += "\\r"; break;
			case '\t': result += "\\t"; break;
			default: result.push_back(c); break;
			}
		}
		return result;
	}

	bool ParseFloat2(const std::string& text, DirectX::XMFLOAT2& outValue)
	{
		float x = outValue.x;
		float y = outValue.y;
		if (TryGetFloatValue(text, "x", x)) outValue.x = x;
		if (TryGetFloatValue(text, "y", y)) outValue.y = y;
		return true;
	}

	bool ParseFloat3(const std::string& text, DirectX::XMFLOAT3& outValue)
	{
		float x = outValue.x;
		float y = outValue.y;
		float z = outValue.z;
		if (TryGetFloatValue(text, "x", x)) outValue.x = x;
		if (TryGetFloatValue(text, "y", y)) outValue.y = y;
		if (TryGetFloatValue(text, "z", z)) outValue.z = z;
		return true;
	}

	void ClampCollider(BossAttackScript::Collider& collider)
	{
		collider.id = ClampIntValue(collider.id, 1, 9999);
		collider.shape = BossAttackScript::NormalizeColliderShape(collider.shape);
		collider.startMode = BossAttackScript::NormalizeColliderStartMode(collider.startMode);
		collider.endMode = BossAttackScript::NormalizeColliderEndMode(collider.endMode);
		collider.startRandomRadius = ClampFloat(collider.startRandomRadius, 0.0f, 20.0f);
		collider.endRandomRadius = ClampFloat(collider.endRandomRadius, 0.0f, 20.0f);
		collider.startSize.x = ClampFloat(collider.startSize.x, 0.05f, 20.0f);
		collider.startSize.y = ClampFloat(collider.startSize.y, 0.05f, 20.0f);
		collider.startSize.z = ClampFloat(collider.startSize.z, 0.05f, 20.0f);
		collider.endSize.x = ClampFloat(collider.endSize.x, 0.05f, 20.0f);
		collider.endSize.y = ClampFloat(collider.endSize.y, 0.05f, 20.0f);
		collider.endSize.z = ClampFloat(collider.endSize.z, 0.05f, 20.0f);
		collider.yawDeg = 0.0f;
		if (collider.shape == BossAttackScript::ColliderShapeCircle)
		{
			collider.startSize.z = collider.startSize.x;
			collider.endSize.z = collider.endSize.x;
		}
		if (!collider.useEndPosition)
		{
			collider.endPos = collider.startPos;
			collider.endSize = collider.startSize;
		}
		else
		{
			collider.startSize.z = collider.startSize.x;
			collider.endSize.z = collider.endSize.x;
		}
		if (collider.name.empty())
		{
			collider.name = "Collider";
		}
	}

	void ClampVisual(BossAttackScript::Visual& visual)
	{
		visual.size.x = ClampFloat(visual.size.x, 0.05f, 20.0f);
		visual.size.y = ClampFloat(visual.size.y, 0.05f, 20.0f);
		visual.spawnHeight = ClampFloat(visual.spawnHeight, 0.0f, 30.0f);
		visual.travelSec = ClampFloat(visual.travelSec, 0.05f, 10.0f);
		visual.spinDegPerSec = ClampFloat(visual.spinDegPerSec, -720.0f, 720.0f);
		if (!visual.texturePath.empty() && visual.texturePath.find("Assets/") != 0)
		{
			visual.texturePath.clear();
		}
	}

	void ClampAttack(BossAttackScript::Attack& attack)
	{
		attack.telegraphSec = ClampFloat(attack.telegraphSec, 0.05f, 20.0f);
		attack.activeSec = ClampFloat(attack.activeSec, 0.05f, 10.0f);
		attack.cooldownSec = ClampFloat(attack.cooldownSec, 0.0f, 20.0f);
		attack.damageScale = ClampFloat(attack.damageScale, 0.0f, 20.0f);
		attack.repeatCount = ClampIntValue(attack.repeatCount, 1, 64);
		attack.repeatIntervalSec = ClampFloat(attack.repeatIntervalSec, 0.0f, 10.0f);
		attack.deliveryMode = BossAttackScript::NormalizeAttackDeliveryMode(attack.deliveryMode);
		attack.movePreset = BossAttackScript::NormalizeMovePreset(attack.movePreset);
		attack.moveSpeed = ClampFloat(attack.moveSpeed, 0.0f, 30.0f);
		attack.moveDistance = ClampFloat(attack.moveDistance, 0.0f, 20.0f);
		attack.spawnMode = BossAttackScript::NormalizeSpawnMode(attack.spawnMode);
		attack.spawnCount = ClampIntValue(attack.spawnCount, 1, 64);
		attack.randomRadius = ClampFloat(attack.randomRadius, 0.0f, 20.0f);
		attack.followStrength = ClampFloat(attack.followStrength, 0.0f, 1.0f);
		ClampVisual(attack.visual);
		if (attack.name.empty())
		{
			attack.name = "Attack";
		}
		std::sort(attack.colliderIds.begin(), attack.colliderIds.end());
		attack.colliderIds.erase(std::unique(attack.colliderIds.begin(), attack.colliderIds.end()), attack.colliderIds.end());
	}

	void ClampProfile(BossAttackScript::Profile& profile)
	{
		profile.type = BossAttackScript::NormalizeProfileType(profile.type);
		profile.hpScale = ClampFloat(profile.hpScale, 0.10f, 8.0f);
		profile.guardScale = ClampFloat(profile.guardScale, 0.10f, 8.0f);
		profile.sizeScale = ClampFloat(profile.sizeScale, 0.20f, 3.0f);
		profile.cooldownScale = ClampFloat(profile.cooldownScale, 0.10f, 4.0f);
		profile.telegraphScale = ClampFloat(profile.telegraphScale, 0.10f, 4.0f);
		profile.damageScale = ClampFloat(profile.damageScale, 0.10f, 8.0f);
		for (auto& collider : profile.colliders)
		{
			ClampCollider(collider);
		}
		for (auto& attack : profile.attacks)
		{
			ClampAttack(attack);
		}
		if (profile.displayName.empty())
		{
			profile.displayName = BossAttackScript::GetProfileName(profile.type);
		}
	}

	void ClampDatabase(BossAttackScript::Database& database)
	{
		for (auto& profile : database.profiles)
		{
			ClampProfile(profile);
		}

		for (int type = 0; type < BossAttackScript::ProfileTypeCount; ++type)
		{
			if (!BossAttackScript::FindProfile(database, type))
			{
				database.profiles.push_back(BossAttackScript::MakeDefaultProfile(type));
			}
		}

		std::sort(database.profiles.begin(), database.profiles.end(), [](const BossAttackScript::Profile& a, const BossAttackScript::Profile& b)
		{
			return a.type < b.type;
		});
	}

	std::string SerializeFloat2(const DirectX::XMFLOAT2& value)
	{
		std::ostringstream oss;
		oss << "{ \"x\": " << value.x << ", \"y\": " << value.y << " }";
		return oss.str();
	}

	std::string SerializeFloat3(const DirectX::XMFLOAT3& value)
	{
		std::ostringstream oss;
		oss << "{ \"x\": " << value.x << ", \"y\": " << value.y << ", \"z\": " << value.z << " }";
		return oss.str();
	}

	bool ParseCollider(const std::string& text, BossAttackScript::Collider& outCollider)
	{
		TryGetIntValue(text, "id", outCollider.id);
		TryGetBoolValue(text, "enabled", outCollider.enabled);
		TryGetStringValue(text, "name", outCollider.name);
		TryGetIntValue(text, "shape", outCollider.shape);
		TryGetIntValue(text, "startMode", outCollider.startMode);
		TryGetFloatValue(text, "startRandomRadius", outCollider.startRandomRadius);
		TryGetBoolValue(text, "useEndPosition", outCollider.useEndPosition);
		const bool hasEndMode = TryGetIntValue(text, "endMode", outCollider.endMode);
		std::string value;
		if (TryGetObjectValue(text, "startPos", value))
		{
			ParseFloat3(value, outCollider.startPos);
		}
		else if (TryGetObjectValue(text, "offset", value))
		{
			ParseFloat3(value, outCollider.startPos);
		}
		if (TryGetObjectValue(text, "endPos", value))
		{
			ParseFloat3(value, outCollider.endPos);
		}
		if (TryGetObjectValue(text, "startSize", value))
		{
			ParseFloat3(value, outCollider.startSize);
		}
		else if (TryGetObjectValue(text, "size", value))
		{
			ParseFloat3(value, outCollider.startSize);
			outCollider.endSize = outCollider.startSize;
		}
		if (TryGetObjectValue(text, "endSize", value))
		{
			ParseFloat3(value, outCollider.endSize);
		}
		TryGetFloatValue(text, "endRandomRadius", outCollider.endRandomRadius);
		if (!hasEndMode)
		{
			outCollider.endMode = (outCollider.startMode == BossAttackScript::ColliderStartCurrent)
				? BossAttackScript::ColliderEndCurrentRelative
				: BossAttackScript::ColliderEndAbsolute;
		}
		TryGetFloatValue(text, "yawDeg", outCollider.yawDeg);
		ClampCollider(outCollider);
		return true;
	}

	bool ParseVisual(const std::string& text, BossAttackScript::Visual& outVisual)
	{
		TryGetBoolValue(text, "enabled", outVisual.enabled);
		TryGetBoolValue(text, "billboard", outVisual.billboard);
		TryGetStringValue(text, "texturePath", outVisual.texturePath);
		std::string value;
		if (TryGetObjectValue(text, "size", value))
		{
			ParseFloat2(value, outVisual.size);
		}
		TryGetFloatValue(text, "spawnHeight", outVisual.spawnHeight);
		TryGetFloatValue(text, "travelSec", outVisual.travelSec);
		TryGetFloatValue(text, "spinDegPerSec", outVisual.spinDegPerSec);
		ClampVisual(outVisual);
		return true;
	}

	bool ParseIntArray(const std::string& text, std::vector<int>& outValues)
	{
		outValues.clear();
		for (const std::string& token : SplitTopLevelTokens(text))
		{
			int value = 0;
			if (TryParseInt(token, value))
			{
				outValues.push_back(value);
			}
		}
		return true;
	}

	bool ParseAttack(const std::string& text, BossAttackScript::Attack& outAttack)
	{
		TryGetBoolValue(text, "enabled", outAttack.enabled);
		TryGetStringValue(text, "name", outAttack.name);
		TryGetFloatValue(text, "telegraphSec", outAttack.telegraphSec);
		TryGetFloatValue(text, "activeSec", outAttack.activeSec);
		TryGetFloatValue(text, "cooldownSec", outAttack.cooldownSec);
		TryGetFloatValue(text, "damageScale", outAttack.damageScale);
		TryGetIntValue(text, "repeatCount", outAttack.repeatCount);
		TryGetFloatValue(text, "repeatIntervalSec", outAttack.repeatIntervalSec);
		TryGetIntValue(text, "deliveryMode", outAttack.deliveryMode);
		TryGetIntValue(text, "movePreset", outAttack.movePreset);
		TryGetFloatValue(text, "moveSpeed", outAttack.moveSpeed);
		TryGetFloatValue(text, "moveDistance", outAttack.moveDistance);
		TryGetIntValue(text, "spawnMode", outAttack.spawnMode);
		TryGetIntValue(text, "spawnCount", outAttack.spawnCount);
		TryGetFloatValue(text, "randomRadius", outAttack.randomRadius);
		TryGetFloatValue(text, "followStrength", outAttack.followStrength);

		std::string value;
		if (TryGetArrayValue(text, "colliderIds", value))
		{
			ParseIntArray(value, outAttack.colliderIds);
		}
		if (TryGetObjectValue(text, "visual", value))
		{
			ParseVisual(value, outAttack.visual);
		}
		ClampAttack(outAttack);
		return true;
	}

	bool ParseProfile(const std::string& text, BossAttackScript::Profile& outProfile)
	{
		TryGetIntValue(text, "type", outProfile.type);
		TryGetStringValue(text, "displayName", outProfile.displayName);
		TryGetFloatValue(text, "hpScale", outProfile.hpScale);
		TryGetFloatValue(text, "guardScale", outProfile.guardScale);
		TryGetFloatValue(text, "sizeScale", outProfile.sizeScale);
		TryGetFloatValue(text, "cooldownScale", outProfile.cooldownScale);
		TryGetFloatValue(text, "telegraphScale", outProfile.telegraphScale);
		TryGetFloatValue(text, "damageScale", outProfile.damageScale);
		TryGetBoolValue(text, "startsSpecial", outProfile.startsSpecial);
		TryGetBoolValue(text, "loops", outProfile.loops);

		std::string arrayText;
		if (TryGetArrayValue(text, "colliders", arrayText))
		{
			outProfile.colliders.clear();
			for (const std::string& objectText : SplitTopLevelObjects(arrayText))
			{
				BossAttackScript::Collider collider;
				ParseCollider(objectText, collider);
				outProfile.colliders.push_back(collider);
			}
		}

		if (TryGetArrayValue(text, "attacks", arrayText))
		{
			outProfile.attacks.clear();
			for (const std::string& objectText : SplitTopLevelObjects(arrayText))
			{
				BossAttackScript::Attack attack;
				ParseAttack(objectText, attack);
				outProfile.attacks.push_back(attack);
			}
		}

		ClampProfile(outProfile);
		return true;
	}
}

int BossAttackScript::NormalizeProfileType(int profileType)
{
	return ClampIntValue(profileType, 0, ProfileTypeCount - 1);
}

int BossAttackScript::NormalizeSpawnMode(int spawnMode)
{
	return ClampIntValue(spawnMode, 0, SpawnModeCount - 1);
}

int BossAttackScript::NormalizeMovePreset(int movePreset)
{
	return ClampIntValue(movePreset, 0, MovePresetCount - 1);
}

int BossAttackScript::NormalizeAttackDeliveryMode(int deliveryMode)
{
	return ClampIntValue(deliveryMode, 0, AttackDeliveryModeCount - 1);
}

int BossAttackScript::NormalizeColliderStartMode(int startMode)
{
	return ClampIntValue(startMode, 0, ColliderStartModeCount - 1);
}

int BossAttackScript::NormalizeColliderEndMode(int endMode)
{
	return ClampIntValue(endMode, 0, ColliderEndModeCount - 1);
}

int BossAttackScript::NormalizeColliderShape(int shape)
{
	return ClampIntValue(shape, 0, ColliderShapeCount - 1);
}

const char* BossAttackScript::GetProfileName(int profileType)
{
	switch (NormalizeProfileType(profileType))
	{
	case ProfileHeavyMelee: return "Heavy Melee";
	case ProfileLightRanged: return "Light Ranged";
	case ProfileBalancedMid: return "Balanced Mid";
	case ProfileSwiftDebuff: return "Swift Debuff";
	case ProfileFinalBarrage: default: return "Final Barrage";
	}
}

const char* BossAttackScript::GetProfileNameJp(int profileType)
{
	switch (NormalizeProfileType(profileType))
	{
	case ProfileHeavyMelee: return u8"近距離鈍重型";
	case ProfileLightRanged: return u8"遠距離軽装型";
	case ProfileBalancedMid: return u8"中距離バランス型";
	case ProfileSwiftDebuff: return u8"近距離俊敏デバフ型";
	case ProfileFinalBarrage: default: return u8"ラスボス";
	}
}

const char* BossAttackScript::GetSpawnModeName(int spawnMode)
{
	switch (NormalizeSpawnMode(spawnMode))
	{
	case SpawnFixed: return "Fixed";
	case SpawnArenaRandom: return "Arena Random";
	case SpawnPlayerAreaRandom: return "Player Area Random";
	case SpawnPlayerPosition: default: return "Player Position";
	}
}

const char* BossAttackScript::GetSpawnModeNameJp(int spawnMode)
{
	switch (NormalizeSpawnMode(spawnMode))
	{
	case SpawnFixed: return u8"固定";
	case SpawnArenaRandom: return u8"完全ランダム";
	case SpawnPlayerAreaRandom: return u8"プレイヤー周辺ランダム";
	case SpawnPlayerPosition: default: return u8"プレイヤー位置";
	}
}

const char* BossAttackScript::GetMovePresetName(int movePreset)
{
	switch (NormalizeMovePreset(movePreset))
	{
	case MoveNone: return "None";
	case MoveFacePlayer: return "Face Player";
	case MoveForward: return "Forward";
	case MoveChargePlayer: return "Charge Player";
	case MoveRetreat: return "Retreat";
	case MoveWarpBehindPlayer: return "Warp Behind";
	case MoveToAttackOrigin: default: return "Move To Origin";
	}
}

const char* BossAttackScript::GetMovePresetNameJp(int movePreset)
{
	switch (NormalizeMovePreset(movePreset))
	{
	case MoveNone: return u8"その場停止";
	case MoveFacePlayer: return u8"プレイヤーを向く";
	case MoveForward: return u8"前進";
	case MoveChargePlayer: return u8"プレイヤーへ突進";
	case MoveRetreat: return u8"後退";
	case MoveWarpBehindPlayer: return u8"背後ワープ";
	case MoveToAttackOrigin: default: return u8"攻撃地点へ移動";
	}
}

const char* BossAttackScript::GetAttackDeliveryName(int deliveryMode)
{
	switch (NormalizeAttackDeliveryMode(deliveryMode))
	{
	case AttackDeliveryRemoteFalling: return "Remote Falling";
	case AttackDeliveryRemoteGround: return "Remote Ground";
	case AttackDeliveryBossSelf: default: return "Boss Self";
	}
}

const char* BossAttackScript::GetAttackDeliveryNameJp(int deliveryMode)
{
	switch (NormalizeAttackDeliveryMode(deliveryMode))
	{
	case AttackDeliveryRemoteFalling: return u8"遠隔(落下)";
	case AttackDeliveryRemoteGround: return u8"遠隔(地上)";
	case AttackDeliveryBossSelf: default: return u8"ボス自身";
	}
}

const char* BossAttackScript::GetColliderStartModeName(int startMode)
{
	switch (NormalizeColliderStartMode(startMode))
	{
	case ColliderStartCurrent: return "From Current";
	case ColliderStartPlayer: return "Player Position";
	case ColliderStartPlayerAreaRandom: return "Player Area Random";
	case ColliderStartAbsolute: default: return "Absolute";
	}
}

const char* BossAttackScript::GetColliderStartModeNameJp(int startMode)
{
	switch (NormalizeColliderStartMode(startMode))
	{
	case ColliderStartCurrent: return u8"現在地から";
	case ColliderStartPlayer: return u8"プレイヤー位置";
	case ColliderStartPlayerAreaRandom: return u8"プレイヤー周辺ランダム";
	case ColliderStartAbsolute: default: return u8"座標指定";
	}
}

const char* BossAttackScript::GetColliderEndModeName(int endMode)
{
	switch (NormalizeColliderEndMode(endMode))
	{
	case ColliderEndCurrentRelative: return "From Current";
	case ColliderEndPlayer: return "Player Position";
	case ColliderEndPlayerAreaRandom: return "Player Area Random";
	case ColliderEndAbsolute: default: return "Absolute";
	}
}

const char* BossAttackScript::GetColliderEndModeNameJp(int endMode)
{
	switch (NormalizeColliderEndMode(endMode))
	{
	case ColliderEndCurrentRelative: return u8"現在地から";
	case ColliderEndPlayer: return u8"プレイヤー位置";
	case ColliderEndPlayerAreaRandom: return u8"プレイヤー周辺ランダム";
	case ColliderEndAbsolute: default: return u8"座標指定";
	}
}

const char* BossAttackScript::GetColliderShapeName(int shape)
{
	switch (NormalizeColliderShape(shape))
	{
	case ColliderShapeCircle: return "Circle";
	case ColliderShapeBox:
	default: return "Box";
	}
}

const char* BossAttackScript::GetColliderShapeNameJp(int shape)
{
	switch (NormalizeColliderShape(shape))
	{
	case ColliderShapeCircle: return u8"円形";
	case ColliderShapeBox:
	default: return u8"Box";
	}
}

BossAttackScript::Profile BossAttackScript::MakeDefaultProfile(int profileType)
{
	Profile profile;
	profile.type = NormalizeProfileType(profileType);
	profile.displayName = GetProfileName(profile.type);
	auto makeVisual = [](const char* texturePath,
		float sizeX,
		float sizeY,
		float spawnHeight,
		float travelSec,
		float spinDegPerSec,
		bool billboard = true)
	{
		Visual visual;
		visual.enabled = true;
		visual.billboard = billboard;
		visual.texturePath = texturePath;
		visual.size = { sizeX, sizeY };
		visual.spawnHeight = spawnHeight;
		visual.travelSec = travelSec;
		visual.spinDegPerSec = spinDegPerSec;
		return visual;
	};
	auto makeAttack = [](const char* name,
		float telegraphSec,
		float activeSec,
		float cooldownSec,
		float damageScale,
		int repeatCount,
		float repeatIntervalSec,
		int deliveryMode,
		int movePreset,
		float moveSpeed,
		float moveDistance,
		int spawnMode,
		int spawnCount,
		float randomRadius,
		std::initializer_list<int> colliderIds,
		const Visual& visual = Visual{})
	{
		Attack attack;
		attack.enabled = true;
		attack.name = name;
		attack.telegraphSec = telegraphSec;
		attack.activeSec = activeSec;
		attack.cooldownSec = cooldownSec;
		attack.damageScale = damageScale;
		attack.repeatCount = repeatCount;
		attack.repeatIntervalSec = repeatIntervalSec;
		attack.deliveryMode = deliveryMode;
		attack.movePreset = movePreset;
		attack.moveSpeed = moveSpeed;
		attack.moveDistance = moveDistance;
		attack.spawnMode = spawnMode;
		attack.spawnCount = spawnCount;
		attack.randomRadius = randomRadius;
		attack.colliderIds.assign(colliderIds.begin(), colliderIds.end());
		attack.visual = visual;
		return attack;
	};

	Collider narrowLane;
	narrowLane.id = 1;
	narrowLane.name = "Front Slash";
	narrowLane.startMode = ColliderStartCurrent;
	narrowLane.startPos = { 0.0f, 0.05f, 1.8f };
	narrowLane.useEndPosition = false;
	narrowLane.endMode = ColliderEndCurrentRelative;
	narrowLane.endPos = narrowLane.startPos;
	narrowLane.startSize = { 1.3f, 0.10f, 4.0f };
	narrowLane.endSize = narrowLane.startSize;

	Collider wideSwing;
	wideSwing.id = 2;
	wideSwing.name = "Wide Sweep";
	wideSwing.startMode = ColliderStartCurrent;
	wideSwing.startPos = { 0.0f, 0.05f, 0.0f };
	wideSwing.useEndPosition = false;
	wideSwing.endMode = ColliderEndCurrentRelative;
	wideSwing.endPos = wideSwing.startPos;
	wideSwing.startSize = { 4.0f, 0.10f, 4.0f };
	wideSwing.endSize = wideSwing.startSize;

	Collider dropZone;
	dropZone.id = 3;
	dropZone.name = "Drop Zone";
	dropZone.startMode = ColliderStartCurrent;
	dropZone.startPos = { 0.0f, 0.05f, 0.0f };
	dropZone.useEndPosition = false;
	dropZone.endMode = ColliderEndCurrentRelative;
	dropZone.endPos = dropZone.startPos;
	dropZone.startSize = { 1.4f, 0.10f, 1.4f };
	dropZone.endSize = dropZone.startSize;

	profile.colliders = { narrowLane, wideSwing, dropZone };

	switch (profile.type)
	{
	case ProfileHeavyMelee:
		profile.hpScale = 1.25f;
		profile.guardScale = 1.35f;
		profile.sizeScale = 1.18f;
		profile.cooldownScale = 1.12f;
		profile.telegraphScale = 1.08f;
		profile.damageScale = 1.20f;
		profile.loops = false;
		profile.attacks.push_back(makeAttack("Heavy Slash", 0.85f, 0.20f, 1.10f, 1.20f, 1, 0.0f,
			AttackDeliveryBossSelf, MoveChargePlayer, 4.0f, 2.6f, SpawnFixed, 1, 0.0f, { 1 }));
		profile.attacks.push_back(makeAttack("Ground Crush", 1.05f, 0.24f, 1.35f, 1.35f, 1, 0.0f,
			AttackDeliveryRemoteGround, MoveNone, 0.0f, 0.0f, SpawnPlayerAreaRandom, 1, 1.6f, { 2 }));
		break;

	case ProfileLightRanged:
		profile.hpScale = 0.88f;
		profile.guardScale = 0.82f;
		profile.sizeScale = 0.88f;
		profile.cooldownScale = 0.82f;
		profile.telegraphScale = 0.88f;
		profile.damageScale = 0.86f;
		profile.loops = false;
		profile.attacks.push_back(makeAttack("Random Rain", 0.70f, 0.16f, 0.85f, 0.85f, 1, 0.0f,
			AttackDeliveryRemoteFalling, MoveNone, 0.0f, 0.0f, SpawnArenaRandom, 4, 4.0f, { 3 },
			makeVisual("Assets/Texture/Game/rock.png", 0.9f, 0.9f, 2.6f, 0.35f, 120.0f)));
		profile.attacks.push_back(makeAttack("Track Rain", 0.60f, 0.16f, 0.70f, 0.92f, 3, 0.20f,
			AttackDeliveryRemoteFalling, MoveNone, 0.0f, 0.0f, SpawnPlayerAreaRandom, 1, 1.3f, { 3 },
			makeVisual("Assets/Texture/Game/rock.png", 0.85f, 0.85f, 2.8f, 0.30f, 180.0f)));
		break;

	case ProfileSwiftDebuff:
		profile.hpScale = 0.95f;
		profile.guardScale = 0.90f;
		profile.sizeScale = 0.92f;
		profile.cooldownScale = 0.68f;
		profile.telegraphScale = 0.76f;
		profile.damageScale = 0.95f;
		profile.loops = false;
		profile.attacks.push_back(makeAttack("Back Warp Slash", 0.55f, 0.18f, 0.75f, 1.05f, 1, 0.0f,
			AttackDeliveryBossSelf, MoveWarpBehindPlayer, 0.0f, 1.4f, SpawnFixed, 1, 0.0f, { 1 }));
		profile.attacks.push_back(makeAttack("Curse Rain", 0.50f, 0.16f, 0.75f, 0.88f, 2, 0.18f,
			AttackDeliveryRemoteFalling, MoveNone, 0.0f, 0.0f, SpawnPlayerAreaRandom, 2, 1.8f, { 3 },
			makeVisual("Assets/Texture/Game/rock.png", 0.75f, 0.75f, 2.4f, 0.28f, 260.0f)));
		break;

	case ProfileFinalBarrage:
		profile.hpScale = 1.75f;
		profile.guardScale = 1.45f;
		profile.sizeScale = 1.08f;
		profile.cooldownScale = 0.55f;
		profile.telegraphScale = 0.65f;
		profile.damageScale = 1.15f;
		profile.startsSpecial = true;
		profile.loops = true;
		profile.attacks.push_back(makeAttack("Dense Rain", 0.60f, 0.16f, 0.30f, 0.90f, 1, 0.0f,
			AttackDeliveryRemoteFalling, MoveNone, 0.0f, 0.0f, SpawnArenaRandom, 8, 4.5f, { 3 },
			makeVisual("Assets/Texture/Game/rock.png", 0.95f, 0.95f, 3.0f, 0.30f, 180.0f)));
		profile.attacks.push_back(makeAttack("Tracking Barrage", 0.45f, 0.16f, 0.25f, 0.96f, 5, 0.12f,
			AttackDeliveryRemoteFalling, MoveNone, 0.0f, 0.0f, SpawnPlayerAreaRandom, 2, 1.0f, { 3 },
			makeVisual("Assets/Texture/Game/rock.png", 0.80f, 0.80f, 2.6f, 0.24f, 220.0f)));
		profile.attacks.push_back(makeAttack("Arena Burst", 0.80f, 0.20f, 0.40f, 1.25f, 1, 0.0f,
			AttackDeliveryRemoteGround, MoveNone, 0.0f, 0.0f, SpawnPlayerAreaRandom, 1, 1.6f, { 2 }));
		break;

	case ProfileBalancedMid:
	default:
		profile.hpScale = 1.0f;
		profile.guardScale = 1.0f;
		profile.sizeScale = 1.0f;
		profile.cooldownScale = 1.0f;
		profile.telegraphScale = 1.0f;
		profile.damageScale = 1.0f;
		profile.loops = false;
		profile.attacks.push_back(makeAttack("Mid Slash", 0.70f, 0.18f, 0.95f, 1.00f, 1, 0.0f,
			AttackDeliveryBossSelf, MoveForward, 4.0f, 1.8f, SpawnFixed, 1, 0.0f, { 1 }));
		profile.attacks.push_back(makeAttack("Mid Rain", 0.75f, 0.16f, 0.85f, 0.92f, 2, 0.18f,
			AttackDeliveryRemoteFalling, MoveNone, 0.0f, 0.0f, SpawnPlayerAreaRandom, 1, 1.6f, { 3 },
			makeVisual("Assets/Texture/Game/rock.png", 0.85f, 0.85f, 2.8f, 0.32f, 140.0f)));
		break;
	}

	for (auto& attack : profile.attacks)
	{
		ClampAttack(attack);
	}
	ClampProfile(profile);
	return profile;
}

BossAttackScript::Database BossAttackScript::MakeDefaultDatabase()
{
	Database database;
	for (int profileType = 0; profileType < ProfileTypeCount; ++profileType)
	{
		database.profiles.push_back(MakeDefaultProfile(profileType));
	}
	return database;
}

const BossAttackScript::Profile* BossAttackScript::FindProfile(const Database& database, int profileType)
{
	const int normalizedType = NormalizeProfileType(profileType);
	for (const auto& profile : database.profiles)
	{
		if (profile.type == normalizedType)
		{
			return &profile;
		}
	}
	return nullptr;
}

BossAttackScript::Profile* BossAttackScript::FindProfile(Database& database, int profileType)
{
	const int normalizedType = NormalizeProfileType(profileType);
	for (auto& profile : database.profiles)
	{
		if (profile.type == normalizedType)
		{
			return &profile;
		}
	}
	return nullptr;
}

const BossAttackScript::Collider* BossAttackScript::FindCollider(const Profile& profile, int colliderId)
{
	for (const auto& collider : profile.colliders)
	{
		if (collider.id == colliderId)
		{
			return &collider;
		}
	}
	return nullptr;
}

bool BossAttackScript::Load(Database& outDatabase, const char* path)
{
	std::ifstream ifs(ResolvePath(path), std::ios::in | std::ios::binary);
	if (!ifs)
	{
		outDatabase = MakeDefaultDatabase();
		return false;
	}

	std::ostringstream buffer;
	buffer << ifs.rdbuf();
	std::string text = buffer.str();
	if (text.empty())
	{
		outDatabase = MakeDefaultDatabase();
		return false;
	}

	Database database = MakeDefaultDatabase();
	std::string profilesArray;
	if (TryGetArrayValue(text, "profiles", profilesArray))
	{
		database.profiles.clear();
		for (const std::string& objectText : SplitTopLevelObjects(profilesArray))
		{
			Profile profile = MakeDefaultProfile(ProfileHeavyMelee);
			ParseProfile(objectText, profile);
			database.profiles.push_back(profile);
		}
	}

	ClampDatabase(database);
	outDatabase = std::move(database);
	return true;
}

bool BossAttackScript::Save(const Database& database, const char* path)
{
	Database clamped = database;
	ClampDatabase(clamped);

	std::ostringstream oss;
	oss << "{\n";
	oss << "  \"version\": " << kCurrentVersion << ",\n";
	oss << "  \"profiles\": [\n";
	for (size_t i = 0; i < clamped.profiles.size(); ++i)
	{
		const Profile& profile = clamped.profiles[i];
		oss << "    {\n";
		oss << "      \"type\": " << profile.type << ",\n";
		oss << "      \"displayName\": \"" << EscapeJson(profile.displayName) << "\",\n";
		oss << "      \"hpScale\": " << profile.hpScale << ",\n";
		oss << "      \"guardScale\": " << profile.guardScale << ",\n";
		oss << "      \"sizeScale\": " << profile.sizeScale << ",\n";
		oss << "      \"cooldownScale\": " << profile.cooldownScale << ",\n";
		oss << "      \"telegraphScale\": " << profile.telegraphScale << ",\n";
		oss << "      \"damageScale\": " << profile.damageScale << ",\n";
		oss << "      \"startsSpecial\": " << (profile.startsSpecial ? "true" : "false") << ",\n";
		oss << "      \"loops\": " << (profile.loops ? "true" : "false") << ",\n";
		oss << "      \"colliders\": [\n";
		for (size_t colliderIndex = 0; colliderIndex < profile.colliders.size(); ++colliderIndex)
		{
			const Collider& collider = profile.colliders[colliderIndex];
			oss << "        {\n";
			oss << "          \"id\": " << collider.id << ",\n";
			oss << "          \"enabled\": " << (collider.enabled ? "true" : "false") << ",\n";
			oss << "          \"name\": \"" << EscapeJson(collider.name) << "\",\n";
			oss << "          \"shape\": " << collider.shape << ",\n";
			oss << "          \"startMode\": " << collider.startMode << ",\n";
			oss << "          \"startPos\": " << SerializeFloat3(collider.startPos) << ",\n";
			oss << "          \"startRandomRadius\": " << collider.startRandomRadius << ",\n";
			oss << "          \"useEndPosition\": " << (collider.useEndPosition ? "true" : "false") << ",\n";
			oss << "          \"endMode\": " << collider.endMode << ",\n";
			oss << "          \"endPos\": " << SerializeFloat3(collider.endPos) << ",\n";
			oss << "          \"endRandomRadius\": " << collider.endRandomRadius << ",\n";
			oss << "          \"startSize\": " << SerializeFloat3(collider.startSize) << ",\n";
			oss << "          \"endSize\": " << SerializeFloat3(collider.endSize) << ",\n";
			oss << "          \"yawDeg\": " << collider.yawDeg << "\n";
			oss << "        }" << ((colliderIndex + 1 < profile.colliders.size()) ? "," : "") << "\n";
		}
		oss << "      ],\n";
		oss << "      \"attacks\": [\n";
		for (size_t attackIndex = 0; attackIndex < profile.attacks.size(); ++attackIndex)
		{
			const Attack& attack = profile.attacks[attackIndex];
			oss << "        {\n";
			oss << "          \"enabled\": " << (attack.enabled ? "true" : "false") << ",\n";
			oss << "          \"name\": \"" << EscapeJson(attack.name) << "\",\n";
			oss << "          \"telegraphSec\": " << attack.telegraphSec << ",\n";
			oss << "          \"activeSec\": " << attack.activeSec << ",\n";
			oss << "          \"cooldownSec\": " << attack.cooldownSec << ",\n";
			oss << "          \"damageScale\": " << attack.damageScale << ",\n";
			oss << "          \"repeatCount\": " << attack.repeatCount << ",\n";
			oss << "          \"repeatIntervalSec\": " << attack.repeatIntervalSec << ",\n";
			oss << "          \"deliveryMode\": " << attack.deliveryMode << ",\n";
			oss << "          \"movePreset\": " << attack.movePreset << ",\n";
			oss << "          \"moveSpeed\": " << attack.moveSpeed << ",\n";
			oss << "          \"moveDistance\": " << attack.moveDistance << ",\n";
			oss << "          \"spawnMode\": " << attack.spawnMode << ",\n";
			oss << "          \"spawnCount\": " << attack.spawnCount << ",\n";
			oss << "          \"randomRadius\": " << attack.randomRadius << ",\n";
			oss << "          \"followStrength\": " << attack.followStrength << ",\n";
			oss << "          \"colliderIds\": [";
			for (size_t idIndex = 0; idIndex < attack.colliderIds.size(); ++idIndex)
			{
				oss << attack.colliderIds[idIndex];
				if (idIndex + 1 < attack.colliderIds.size())
				{
					oss << ", ";
				}
			}
			oss << "],\n";
			oss << "          \"visual\": {\n";
			oss << "            \"enabled\": " << (attack.visual.enabled ? "true" : "false") << ",\n";
			oss << "            \"billboard\": " << (attack.visual.billboard ? "true" : "false") << ",\n";
			oss << "            \"texturePath\": \"" << EscapeJson(attack.visual.texturePath) << "\",\n";
			oss << "            \"size\": " << SerializeFloat2(attack.visual.size) << ",\n";
			oss << "            \"spawnHeight\": " << attack.visual.spawnHeight << ",\n";
			oss << "            \"travelSec\": " << attack.visual.travelSec << ",\n";
			oss << "            \"spinDegPerSec\": " << attack.visual.spinDegPerSec << "\n";
			oss << "          }\n";
			oss << "        }" << ((attackIndex + 1 < profile.attacks.size()) ? "," : "") << "\n";
		}
		oss << "      ]\n";
		oss << "    }" << ((i + 1 < clamped.profiles.size()) ? "," : "") << "\n";
	}
	oss << "  ]\n";
	oss << "}\n";

	const char* resolvedPath = ResolvePath(path);
	std::ofstream ofs(resolvedPath, std::ios::out | std::ios::binary | std::ios::trunc);
	if (!ofs)
	{
		return false;
	}
	ofs << oss.str();
	ofs.close();

	if (IsDefaultPathArgument(path))
	{
		const char* mirrorPaths[] = {
			kMirrorBossAttackScriptPath,
			kDebugBossAttackScriptPath,
			kUpstreamMirrorBossAttackScriptPath
		};
		for (const char* mirrorPath : mirrorPaths)
		{
			std::ofstream mirror(mirrorPath, std::ios::out | std::ios::binary | std::ios::trunc);
			if (mirror)
			{
				mirror << oss.str();
			}
		}
	}

	return true;
}

bool BossAttackScript::LoadProfile(Profile& outProfile, int profileType, const char* path)
{
	Database database;
	Load(database, path);
	if (const Profile* profile = FindProfile(database, profileType))
	{
		outProfile = *profile;
		return true;
	}

	outProfile = MakeDefaultProfile(profileType);
	return false;
}

const char* BossAttackScript::GetDefaultPath()
{
	return kDefaultBossAttackScriptPath;
}

