#include "SceneBossEditor.h"

#include <algorithm>
#include <cmath>
#include <cfloat>
#include <cstdio>
#include <cstring>

#include "CameraDebug.h"
#include "Geometory.h"
#include "Input.h"
#include "SceneManager.h"
#include "Sprite.h"
#include "Texture.h"
#include "imgui.h"

namespace
{
	const float kStageSize = 12.0f;
	const float kStageHalf = kStageSize * 0.5f;
	const float kZoneHeight = 0.10f;
	const float kPlayerSpeed = 5.5f;

	const char* SelectLabel(bool useJapanese, const char* en, const char* jp) { return useJapanese ? jp : en; }
	float ClampFloat(float v, float lo, float hi) { return (v < lo) ? lo : ((v > hi) ? hi : v); }
	int ClampIntValue(int v, int lo, int hi) { return (v < lo) ? lo : ((v > hi) ? hi : v); }
	float Clamp01(float v) { return ClampFloat(v, 0.0f, 1.0f); }
	float DegToRad(float deg) { return deg * (DirectX::XM_PI / 180.0f); }
	DirectX::XMFLOAT3 RotateOffsetY(const DirectX::XMFLOAT3& v, float yawDeg);

	float ComputeFacingYawDeg(const DirectX::XMFLOAT3& from, const DirectX::XMFLOAT3& to, float fallbackDeg = 0.0f)
	{
		const float dx = to.x - from.x;
		const float dz = to.z - from.z;
		const float lenSq = dx * dx + dz * dz;
		if (lenSq <= 0.000001f)
		{
			return fallbackDeg;
		}
		return std::atan2(-dx, dz) * (180.0f / DirectX::XM_PI);
	}

	DirectX::XMFLOAT3 ResolvePreviewPoint(
		int mode,
		const DirectX::XMFLOAT3& value,
		float randomRadius,
		const DirectX::XMFLOAT3& anchor,
		const DirectX::XMFLOAT3& playerPos,
		float facingYawDeg)
	{
		switch (mode)
		{
		case BossAttackScript::ColliderStartCurrent:
		{
			const DirectX::XMFLOAT3 offset = RotateOffsetY(value, facingYawDeg);
			return { anchor.x + offset.x, anchor.y + offset.y, anchor.z + offset.z };
		}
		case BossAttackScript::ColliderStartPlayer:
			return playerPos;
		case BossAttackScript::ColliderStartPlayerAreaRandom:
			return { playerPos.x + randomRadius, playerPos.y, playerPos.z };
		case BossAttackScript::ColliderStartAbsolute:
		default:
			return value;
		}
	}

	DirectX::XMFLOAT3 RotateOffsetY(const DirectX::XMFLOAT3& v, float yawDeg)
	{
		const float rad = DegToRad(yawDeg);
		const float c = std::cos(rad);
		const float s = std::sin(rad);
		return { v.x * c - v.z * s, v.y, v.x * s + v.z * c };
	}

	void PrepareLineDraw(CameraDebug* camera)
	{
		if (!camera) return;
		DirectX::XMFLOAT4X4 identity{};
		DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
		Geometory::SetWorld(identity);
		Geometory::SetView(camera->GetViewMatrix());
		Geometory::SetProjection(camera->GetProjectionMatrix());
	}

	void DrawGroundSprite(Texture* texture, const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT2& size, const DirectX::XMFLOAT4& color, float yawDeg = 0.0f)
	{
		if (!texture) return;
		DirectX::XMMATRIX r = DirectX::XMMatrixRotationZ(DegToRad(yawDeg)) * DirectX::XMMatrixRotationX(DirectX::XM_PIDIV2);
		DirectX::XMMATRIX t = DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
		DirectX::XMFLOAT4X4 world{};
		DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranspose(r * t));
		Sprite::SetWorld(world);
		Sprite::SetSize(size);
		Sprite::SetOffset({ 0.0f, 0.0f });
		Sprite::SetUVPos({ 0.0f, 0.0f });
		Sprite::SetUVScale({ 1.0f, 1.0f });
		Sprite::SetColor(color);
		Sprite::SetTexture(texture);
		Sprite::Draw();
	}

	void DrawBillboardSprite(Texture* texture,
		CameraDebug* camera,
		const DirectX::XMFLOAT3& position,
		const DirectX::XMFLOAT2& size,
		const DirectX::XMFLOAT4& color)
	{
		if (!texture) return;
		using namespace DirectX;

		XMMATRIX billboard = XMMatrixIdentity();
		if (camera)
		{
			const XMFLOAT4X4 viewFloat = camera->GetViewMatrix(false);
			const XMMATRIX viewMat = XMLoadFloat4x4(&viewFloat);
			const XMMATRIX inverseView = XMMatrixInverse(nullptr, viewMat);
			XMFLOAT4X4 inverseViewFloat{};
			XMStoreFloat4x4(&inverseViewFloat, inverseView);
			inverseViewFloat._41 = 0.0f;
			inverseViewFloat._42 = 0.0f;
			inverseViewFloat._43 = 0.0f;
			billboard = XMLoadFloat4x4(&inverseViewFloat);
		}

		XMFLOAT4X4 world{};
		XMStoreFloat4x4(&world, XMMatrixTranspose(
			billboard * XMMatrixTranslation(position.x, position.y, position.z)));
		Sprite::SetWorld(world);
		Sprite::SetSize(size);
		Sprite::SetOffset({ 0.0f, 0.0f });
		Sprite::SetUVPos({ 0.0f, 0.0f });
		Sprite::SetUVScale({ 1.0f, 1.0f });
		Sprite::SetColor(color);
		Sprite::SetTexture(texture);
		Sprite::Draw();
	}

	void AddRectOutline(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& size, float yawDeg, const DirectX::XMFLOAT4& color)
	{
		const float hx = size.x * 0.5f;
		const float hz = size.z * 0.5f;
		DirectX::XMFLOAT3 corners[4] =
		{
			{ -hx, center.y, -hz },
			{  hx, center.y, -hz },
			{  hx, center.y,  hz },
			{ -hx, center.y,  hz }
		};
		for (DirectX::XMFLOAT3& corner : corners)
		{
			DirectX::XMFLOAT3 rotated = RotateOffsetY({ corner.x, 0.0f, corner.z }, yawDeg);
			corner.x = center.x + rotated.x;
			corner.z = center.z + rotated.z;
		}
		for (int i = 0; i < 4; ++i)
		{
			const int next = (i + 1) % 4;
			Geometory::AddLine(corners[i], corners[next], color);
		}
	}

	void AddStripOutline(const DirectX::XMFLOAT3& startPos,
		const DirectX::XMFLOAT3& endPos,
		float thickness,
		const DirectX::XMFLOAT4& color)
	{
		const float dx = endPos.x - startPos.x;
		const float dz = endPos.z - startPos.z;
		const float distanceSq = dx * dx + dz * dz;
		if (distanceSq <= 0.000001f)
		{
			AddRectOutline(startPos, { thickness, 0.0f, thickness }, 0.0f, color);
			return;
		}

		const float invLen = 1.0f / std::sqrt(distanceSq);
		const float halfThickness = thickness * 0.5f;
		const float perpX = -dz * invLen * halfThickness;
		const float perpZ = dx * invLen * halfThickness;

		const DirectX::XMFLOAT3 corners[4] =
		{
			{ startPos.x + perpX, startPos.y, startPos.z + perpZ },
			{ startPos.x - perpX, startPos.y, startPos.z - perpZ },
			{ endPos.x - perpX, endPos.y, endPos.z - perpZ },
			{ endPos.x + perpX, endPos.y, endPos.z + perpZ }
		};

		for (int i = 0; i < 4; ++i)
		{
			Geometory::AddLine(corners[i], corners[(i + 1) % 4], color);
		}
	}

	void AddCircleOutline(const DirectX::XMFLOAT3& center,
		float radius,
		const DirectX::XMFLOAT4& color,
		int segments = 24)
	{
		const int clampedSegments = (segments < 8) ? 8 : segments;
		DirectX::XMFLOAT3 previous = {
			center.x + radius,
			center.y,
			center.z
		};
		for (int i = 1; i <= clampedSegments; ++i)
		{
			const float angle = (static_cast<float>(i) / static_cast<float>(clampedSegments)) * DirectX::XM_2PI;
			const DirectX::XMFLOAT3 current = {
				center.x + std::cos(angle) * radius,
				center.y,
				center.z + std::sin(angle) * radius
			};
			Geometory::AddLine(previous, current, color);
			previous = current;
		}
	}

	void AddCapsuleOutline(const DirectX::XMFLOAT3& startPos,
		const DirectX::XMFLOAT3& endPos,
		float radius,
		const DirectX::XMFLOAT4& color)
	{
		const float dx = endPos.x - startPos.x;
		const float dz = endPos.z - startPos.z;
		const float distanceSq = dx * dx + dz * dz;
		if (distanceSq <= 0.000001f)
		{
			AddCircleOutline(startPos, radius, color);
			return;
		}

		const float invLen = 1.0f / std::sqrt(distanceSq);
		const float perpX = -dz * invLen * radius;
		const float perpZ = dx * invLen * radius;
		const DirectX::XMFLOAT3 sideAStart = { startPos.x + perpX, startPos.y, startPos.z + perpZ };
		const DirectX::XMFLOAT3 sideAEnd = { endPos.x + perpX, endPos.y, endPos.z + perpZ };
		const DirectX::XMFLOAT3 sideBStart = { startPos.x - perpX, startPos.y, startPos.z - perpZ };
		const DirectX::XMFLOAT3 sideBEnd = { endPos.x - perpX, endPos.y, endPos.z - perpZ };
		Geometory::AddLine(sideAStart, sideAEnd, color);
		Geometory::AddLine(sideBStart, sideBEnd, color);
		AddCircleOutline(startPos, radius, color);
		AddCircleOutline(endPos, radius, color);
	}

	float DistancePointToSegment2D(const DirectX::XMFLOAT3& point,
		const DirectX::XMFLOAT3& startPos,
		const DirectX::XMFLOAT3& endPos)
	{
		const float segX = endPos.x - startPos.x;
		const float segZ = endPos.z - startPos.z;
		const float lenSq = segX * segX + segZ * segZ;
		if (lenSq <= 0.000001f)
		{
			const float dx = point.x - startPos.x;
			const float dz = point.z - startPos.z;
			return std::sqrt(dx * dx + dz * dz);
		}

		const float toPointX = point.x - startPos.x;
		const float toPointZ = point.z - startPos.z;
		float t = (toPointX * segX + toPointZ * segZ) / lenSq;
		t = ClampFloat(t, 0.0f, 1.0f);
		const float closestX = startPos.x + segX * t;
		const float closestZ = startPos.z + segZ * t;
		const float dx = point.x - closestX;
		const float dz = point.z - closestZ;
		return std::sqrt(dx * dx + dz * dz);
	}

	DirectX::XMFLOAT3 ClampArenaPos(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& size)
	{
		const float hx = size.x * 0.5f;
		const float hz = size.z * 0.5f;
		return {
			ClampFloat(pos.x, -kStageHalf + hx, kStageHalf - hx),
			pos.y,
			ClampFloat(pos.z, -kStageHalf + hz, kStageHalf - hz)
		};
	}

	SceneBossEditor::PreviewZone BuildPreviewZoneFromCollider(
		const BossAttackScript::Collider& collider,
		const DirectX::XMFLOAT3& anchor,
		const DirectX::XMFLOAT3& playerPos,
		float facingYawDeg)
	{
		SceneBossEditor::PreviewZone zone;
		const float baseYawDeg = (collider.startMode == BossAttackScript::ColliderStartCurrent) ? facingYawDeg : 0.0f;
		zone.shape = BossAttackScript::NormalizeColliderShape(collider.shape);

		DirectX::XMFLOAT3 startPos = ResolvePreviewPoint(
			collider.startMode,
			collider.startPos,
			collider.startRandomRadius,
			anchor,
			playerPos,
			facingYawDeg);
		startPos = ClampArenaPos(startPos, collider.startSize);

		DirectX::XMFLOAT3 endPos = startPos;
		if (collider.useEndPosition)
		{
			switch (BossAttackScript::NormalizeColliderEndMode(collider.endMode))
			{
			case BossAttackScript::ColliderEndCurrentRelative:
			{
				const DirectX::XMFLOAT3 endOffset = RotateOffsetY(collider.endPos, facingYawDeg);
				endPos = { anchor.x + endOffset.x, anchor.y + endOffset.y, anchor.z + endOffset.z };
				break;
			}
			case BossAttackScript::ColliderEndPlayer:
				endPos = playerPos;
				break;
			case BossAttackScript::ColliderEndPlayerAreaRandom:
				endPos = { playerPos.x + collider.endRandomRadius, playerPos.y, playerPos.z };
				break;
			case BossAttackScript::ColliderEndAbsolute:
			default:
				endPos = collider.endPos;
				break;
			}
			endPos = ClampArenaPos(endPos, collider.endSize);
		}

		zone.startPos = startPos;
		zone.endPos = endPos;
		zone.hasPath = collider.useEndPosition;

		if (collider.useEndPosition)
		{
			const float dx = endPos.x - startPos.x;
			const float dz = endPos.z - startPos.z;
			const float distance = std::sqrt(dx * dx + dz * dz);
			if (distance > 0.001f)
			{
				zone.center = {
					(startPos.x + endPos.x) * 0.5f,
					(startPos.y + endPos.y) * 0.5f,
					(startPos.z + endPos.z) * 0.5f
				};
				zone.size = {
					std::max(collider.startSize.x, collider.endSize.x),
					std::max(collider.startSize.y, collider.endSize.y),
					(zone.shape == BossAttackScript::ColliderShapeCircle)
						? std::max(collider.startSize.x, collider.endSize.x)
						: distance
				};
				zone.yawDeg = std::atan2(dx, dz) * (180.0f / DirectX::XM_PI);
				return zone;
			}
		}

		zone.center = startPos;
		zone.size = collider.startSize;
		zone.yawDeg = baseYawDeg;
		zone.hasPath = false;
		zone.endPos = zone.startPos;
		return zone;
	}
}

SceneBossEditor::SceneBossEditor()
	: m_pCamera(new CameraDebug())
	, m_pFloorTexture(new Texture())
	, m_pMarkerTexture(new Texture())
	, m_pBossTexture(new Texture())
	, m_pPlayerTexture(new Texture())
	, m_database(BossAttackScript::MakeDefaultDatabase())
	, m_pathBuffer{}
	, m_selectedProfile(BossAttackScript::ProfileHeavyMelee)
	, m_selectedCollider(0)
	, m_selectedAttack(0)
	, m_useJapaneseLabels(true)
	, m_previewPlayerPos({ 0.0f, 0.0f, 2.5f })
	, m_previewPlayerSize({ 0.8f, 1.5f, 0.8f })
	, m_previewBossPos({ 0.0f, 0.0f, -1.5f })
	, m_previewBossStartPos({ 0.0f, 0.0f, -1.5f })
	, m_previewBossTargetPos({ 0.0f, 0.0f, -1.5f })
	, m_previewPlayerFlashTimer(0.0f)
	, m_previewFacingYawDeg(0.0f)
	, m_previewAttackIndex(0)
	, m_previewRepeatRemaining(0)
	, m_previewStateTimer(0.0f)
	, m_previewStateDuration(0.0f)
	, m_previewState(PreviewIdle)
	, m_previewPlaying(false)
{
	if (m_pCamera) m_pCamera->SetPose({ 0.0f, 9.5f, -9.0f }, { 0.0f, 0.0f, 0.0f });
	if (m_pFloorTexture && FAILED(m_pFloorTexture->Create("Assets/Texture/Game/jimen.png"))) { delete m_pFloorTexture; m_pFloorTexture = nullptr; }
	if (m_pMarkerTexture && FAILED(m_pMarkerTexture->Create("Assets/Texture/Star.png"))) { delete m_pMarkerTexture; m_pMarkerTexture = nullptr; }
	if (m_pBossTexture && FAILED(m_pBossTexture->Create("Assets/Texture/Chracter/genbaneko.png"))) { delete m_pBossTexture; m_pBossTexture = nullptr; }
	if (m_pPlayerTexture && FAILED(m_pPlayerTexture->Create("Assets/Texture/Star.png"))) { delete m_pPlayerTexture; m_pPlayerTexture = nullptr; }
	strncpy_s(m_pathBuffer, sizeof(m_pathBuffer), BossAttackScript::GetDefaultPath(), _TRUNCATE);
	BossAttackScript::Load(m_database, m_pathBuffer);
	ClampSelection();
	ResetPreview(true);
}

SceneBossEditor::~SceneBossEditor()
{
	for (auto& entry : m_textureCache) delete entry.second;
	delete m_pPlayerTexture;
	delete m_pBossTexture;
	delete m_pMarkerTexture;
	delete m_pFloorTexture;
	delete m_pCamera;
}

BossAttackScript::Profile* SceneBossEditor::GetSelectedProfile()
{
	return BossAttackScript::FindProfile(m_database, m_selectedProfile);
}

const BossAttackScript::Profile* SceneBossEditor::GetSelectedProfile() const
{
	return BossAttackScript::FindProfile(m_database, m_selectedProfile);
}

Texture* SceneBossEditor::GetTextureForPath(const std::string& relativePath)
{
	if (relativePath.empty() || relativePath.find("Assets/") != 0) return nullptr;
	for (auto& entry : m_textureCache)
	{
		if (entry.first == relativePath) return entry.second;
	}
	Texture* texture = new Texture();
	if (!texture || FAILED(texture->Create(relativePath.c_str())))
	{
		delete texture;
		return nullptr;
	}
	m_textureCache.push_back({ relativePath, texture });
	return texture;
}

void SceneBossEditor::ClampSelection()
{
	for (int i = 0; i < BossAttackScript::ProfileTypeCount; ++i)
	{
		if (!BossAttackScript::FindProfile(m_database, i))
		{
			m_database.profiles.push_back(BossAttackScript::MakeDefaultProfile(i));
		}
	}
	std::sort(m_database.profiles.begin(), m_database.profiles.end(), [](const BossAttackScript::Profile& a, const BossAttackScript::Profile& b)
	{
		return a.type < b.type;
	});

	m_selectedProfile = BossAttackScript::NormalizeProfileType(m_selectedProfile);
	BossAttackScript::Profile* profile = GetSelectedProfile();
	if (!profile)
	{
		m_selectedProfile = BossAttackScript::ProfileHeavyMelee;
		profile = GetSelectedProfile();
	}
	if (!profile) return;
	if (profile->colliders.empty()) profile->colliders.push_back(BossAttackScript::Collider{});
	if (profile->attacks.empty())
	{
		BossAttackScript::Attack attack;
		attack.colliderIds.push_back(profile->colliders.front().id);
		profile->attacks.push_back(attack);
	}
	m_selectedCollider = ClampIntValue(m_selectedCollider, 0, static_cast<int>(profile->colliders.size()) - 1);
	m_selectedAttack = ClampIntValue(m_selectedAttack, 0, static_cast<int>(profile->attacks.size()) - 1);
}

void SceneBossEditor::ResetPreview(bool resetPlayer)
{
	m_previewPlaying = false;
	m_previewZones.clear();
	m_previewVisuals.clear();
	m_previewAttackIndex = m_selectedAttack;
	m_previewRepeatRemaining = 0;
	m_previewState = PreviewIdle;
	m_previewStateTimer = 0.0f;
	m_previewStateDuration = 0.0f;
	m_previewFacingYawDeg = 0.0f;
	m_previewBossPos = { 0.0f, 0.0f, -1.5f };
	m_previewBossStartPos = m_previewBossPos;
	m_previewBossTargetPos = m_previewBossPos;
	if (resetPlayer) m_previewPlayerPos = { 0.0f, 0.0f, 2.5f };
}

void SceneBossEditor::UpdatePreviewPlayer(float dt)
{
	float moveX = 0.0f;
	float moveZ = 0.0f;
	if (IsRawKeyPress('A')) moveX -= 1.0f;
	if (IsRawKeyPress('D')) moveX += 1.0f;
	if (IsRawKeyPress('W')) moveZ += 1.0f;
	if (IsRawKeyPress('S')) moveZ -= 1.0f;
	moveX += GetPadLeftStickX();
	moveZ += GetPadLeftStickY();
	const float lenSq = moveX * moveX + moveZ * moveZ;
	if (lenSq > 0.0001f)
	{
		const float invLen = 1.0f / std::sqrt(lenSq);
		m_previewPlayerPos.x += moveX * invLen * kPlayerSpeed * dt;
		m_previewPlayerPos.z += moveZ * invLen * kPlayerSpeed * dt;
		m_previewPlayerPos = ClampArenaPos(m_previewPlayerPos, m_previewPlayerSize);
	}
	if (m_previewPlayerFlashTimer > 0.0f)
	{
		m_previewPlayerFlashTimer -= dt;
		if (m_previewPlayerFlashTimer < 0.0f) m_previewPlayerFlashTimer = 0.0f;
	}
}

void SceneBossEditor::BuildPreviewZones(const BossAttackScript::Attack& attack,
	std::vector<PreviewZone>& outZones,
	DirectX::XMFLOAT3& outBossTargetPos,
	float& outFacingYawDeg)
{
	outZones.clear();
	const BossAttackScript::Profile* profile = GetSelectedProfile();
	if (!profile)
	{
		outBossTargetPos = m_previewBossPos;
		outFacingYawDeg = 0.0f;
		return;
	}

	const DirectX::XMFLOAT3 playerDir = RotateOffsetY({ 0.0f, 0.0f, 1.0f }, 0.0f);
	const DirectX::XMFLOAT3 towardPlayer = {
		m_previewPlayerPos.x - m_previewBossPos.x,
		0.0f,
		m_previewPlayerPos.z - m_previewBossPos.z
	};
	const float towardLenSq = towardPlayer.x * towardPlayer.x + towardPlayer.z * towardPlayer.z;
	const float invTowardLen = (towardLenSq > 0.000001f) ? (1.0f / std::sqrt(towardLenSq)) : 0.0f;
	const DirectX::XMFLOAT3 dir = (invTowardLen > 0.0f)
		? DirectX::XMFLOAT3{ towardPlayer.x * invTowardLen, 0.0f, towardPlayer.z * invTowardLen }
		: playerDir;
	outFacingYawDeg = std::atan2(-dir.x, dir.z) * (180.0f / DirectX::XM_PI);
	outBossTargetPos = m_previewBossPos;
	const int deliveryMode = BossAttackScript::NormalizeAttackDeliveryMode(attack.deliveryMode);
	const bool isBossSelfAttack = (deliveryMode == BossAttackScript::AttackDeliveryBossSelf);
	const bool isRemoteFallingAttack = (deliveryMode == BossAttackScript::AttackDeliveryRemoteFalling);

	std::vector<DirectX::XMFLOAT3> origins;
	const int spawnCount = ClampIntValue(attack.spawnCount, 1, 64);
	const int spawnMode = BossAttackScript::NormalizeSpawnMode(attack.spawnMode);
	if (spawnMode == BossAttackScript::SpawnArenaRandom)
	{
		for (int i = 0; i < spawnCount; ++i)
		{
			origins.push_back({
				ClampFloat(((static_cast<float>(std::rand() % 2001) / 1000.0f) - 1.0f) * kStageHalf, -kStageHalf, kStageHalf),
				0.0f,
				ClampFloat(((static_cast<float>(std::rand() % 2001) / 1000.0f) - 1.0f) * kStageHalf, -kStageHalf, kStageHalf)
			});
		}
	}
	else if (spawnMode == BossAttackScript::SpawnPlayerAreaRandom)
	{
		for (int i = 0; i < spawnCount; ++i)
		{
			const float angle = static_cast<float>(std::rand() % 360) * (DirectX::XM_PI / 180.0f);
			const float distance = attack.randomRadius;
			origins.push_back(ClampArenaPos({
				m_previewPlayerPos.x + std::cos(angle) * distance,
				0.0f,
				m_previewPlayerPos.z + std::sin(angle) * distance
			}, { 0.6f, 0.1f, 0.6f }));
		}
	}
	else if (spawnMode == BossAttackScript::SpawnPlayerPosition)
	{
		for (int i = 0; i < spawnCount; ++i)
		{
			origins.push_back(m_previewPlayerPos);
		}
	}
	else
	{
		origins.push_back(m_previewBossPos);
	}

	const int movePreset = BossAttackScript::NormalizeMovePreset(attack.movePreset);
	if (isBossSelfAttack && (movePreset == BossAttackScript::MoveForward || movePreset == BossAttackScript::MoveChargePlayer))
	{
		outBossTargetPos = ClampArenaPos({
			m_previewBossPos.x + dir.x * attack.moveDistance,
			0.0f,
			m_previewBossPos.z + dir.z * attack.moveDistance
		}, { 1.8f, 0.1f, 1.8f });
	}
	else if (isBossSelfAttack && movePreset == BossAttackScript::MoveRetreat)
	{
		outBossTargetPos = ClampArenaPos({
			m_previewBossPos.x - dir.x * attack.moveDistance,
			0.0f,
			m_previewBossPos.z - dir.z * attack.moveDistance
		}, { 1.8f, 0.1f, 1.8f });
	}
	else if (isBossSelfAttack && movePreset == BossAttackScript::MoveWarpBehindPlayer)
	{
		outBossTargetPos = ClampArenaPos({
			m_previewPlayerPos.x - dir.x * attack.moveDistance,
			0.0f,
			m_previewPlayerPos.z - dir.z * attack.moveDistance
		}, { 1.8f, 0.1f, 1.8f });
	}
	else if (isBossSelfAttack && movePreset == BossAttackScript::MoveToAttackOrigin && !origins.empty())
	{
		outBossTargetPos = ClampArenaPos(origins.front(), { 1.8f, 0.1f, 1.8f });
	}

	for (const DirectX::XMFLOAT3& origin : origins)
	{
		const DirectX::XMFLOAT3 anchor = (spawnMode == BossAttackScript::SpawnFixed) ? outBossTargetPos : origin;
		for (int colliderId : attack.colliderIds)
		{
			const BossAttackScript::Collider* collider = BossAttackScript::FindCollider(*profile, colliderId);
			if (!collider || !collider->enabled) continue;
			BossAttackScript::Collider workingCollider = *collider;
			if (isRemoteFallingAttack)
			{
				workingCollider.useEndPosition = false;
			}
			PreviewZone zone = BuildPreviewZoneFromCollider(workingCollider, anchor, m_previewPlayerPos, outFacingYawDeg);
			zone.center.y = std::max(zone.center.y, kZoneHeight);
			zone.size.y = std::max(zone.size.y, kZoneHeight);
			outZones.push_back(zone);
		}
	}
}

void SceneBossEditor::UpdatePreviewHit()
{
	for (const PreviewZone& zone : m_previewZones)
	{
		const float playerHalfHeight = m_previewPlayerSize.y * 0.5f;
		const float zoneHalfHeight = zone.size.y * 0.5f;
		if (std::fabs(m_previewPlayerPos.y - zone.center.y) > (playerHalfHeight + zoneHalfHeight))
		{
			continue;
		}

		const int shape = BossAttackScript::NormalizeColliderShape(zone.shape);
		bool hit = false;
		if (shape == BossAttackScript::ColliderShapeCircle)
		{
			const float playerRadius = std::max(m_previewPlayerSize.x, m_previewPlayerSize.z) * 0.5f;
			const float zoneRadius = zone.size.x * 0.5f;
			const float distance = zone.hasPath
				? DistancePointToSegment2D(m_previewPlayerPos, zone.startPos, zone.endPos)
				: DistancePointToSegment2D(m_previewPlayerPos, zone.center, zone.center);
			hit = (distance <= (zoneRadius + playerRadius));
		}
		else
		{
			const float dx = m_previewPlayerPos.x - zone.center.x;
			const float dz = m_previewPlayerPos.z - zone.center.z;
			const float rad = DegToRad(-zone.yawDeg);
			const float c = std::cos(rad);
			const float s = std::sin(rad);
			const float localX = dx * c - dz * s;
			const float localZ = dx * s + dz * c;
			const float limitX = zone.size.x * 0.5f + m_previewPlayerSize.x * 0.5f;
			const float limitZ = zone.size.z * 0.5f + m_previewPlayerSize.z * 0.5f;
			hit = (std::fabs(localX) <= limitX) && (std::fabs(localZ) <= limitZ);
		}

		if (hit)
		{
			m_previewPlayerFlashTimer = 0.18f;
			break;
		}
	}
}

void SceneBossEditor::BeginPreviewAttack(int attackIndex)
{
	const BossAttackScript::Profile* profile = GetSelectedProfile();
	if (!profile || profile->attacks.empty())
	{
		m_previewPlaying = false;
		return;
	}
	const int clampedIndex = ClampIntValue(attackIndex, 0, static_cast<int>(profile->attacks.size()) - 1);
	const BossAttackScript::Attack& attack = profile->attacks[clampedIndex];
	if (!attack.enabled)
	{
		m_previewPlaying = false;
		m_previewState = PreviewIdle;
		m_previewZones.clear();
		return;
	}
	m_previewAttackIndex = clampedIndex;
	m_previewBossStartPos = m_previewBossPos;
	BuildPreviewZones(attack, m_previewZones, m_previewBossTargetPos, m_previewFacingYawDeg);
	m_previewState = PreviewTelegraph;
	m_previewStateTimer = 0.0f;
	m_previewStateDuration = ClampFloat(attack.telegraphSec, 0.05f, 20.0f);
	if (m_previewRepeatRemaining <= 0)
	{
		m_previewRepeatRemaining = ClampIntValue(attack.repeatCount, 1, 64);
	}
}

void SceneBossEditor::AdvancePreviewAttack()
{
	const BossAttackScript::Profile* profile = GetSelectedProfile();
	if (!profile || profile->attacks.empty())
	{
		m_previewPlaying = false;
		m_previewState = PreviewIdle;
		return;
	}
	int candidate = m_previewAttackIndex;
	for (int attempt = 0; attempt < static_cast<int>(profile->attacks.size()); ++attempt)
	{
		candidate = (candidate + 1) % static_cast<int>(profile->attacks.size());
		if (profile->attacks[candidate].enabled)
		{
			m_previewRepeatRemaining = ClampIntValue(profile->attacks[candidate].repeatCount, 1, 64);
			BeginPreviewAttack(candidate);
			return;
		}
	}
	m_previewPlaying = false;
	m_previewState = PreviewIdle;
}

void SceneBossEditor::UpdatePreviewVisuals(float dt)
{
	for (PreviewVisual& visual : m_previewVisuals)
	{
		visual.timer -= dt;
		visual.angleDeg += visual.spinDegPerSec * dt;
	}
	m_previewVisuals.erase(std::remove_if(m_previewVisuals.begin(), m_previewVisuals.end(), [](const PreviewVisual& visual)
	{
		return visual.timer <= 0.0f;
	}), m_previewVisuals.end());
}

void SceneBossEditor::UpdatePreviewPlayback(float dt)
{
	UpdatePreviewVisuals(dt);
	if (!m_previewPlaying) return;
	const BossAttackScript::Profile* profile = GetSelectedProfile();
	if (!profile || m_previewAttackIndex < 0 || m_previewAttackIndex >= static_cast<int>(profile->attacks.size()))
	{
		ResetPreview(false);
		return;
	}
	const BossAttackScript::Attack& attack = profile->attacks[m_previewAttackIndex];
	m_previewStateTimer += dt;
	if (m_previewState == PreviewTelegraph)
	{
		if (m_previewStateTimer >= m_previewStateDuration)
		{
			m_previewState = PreviewActive;
			m_previewStateTimer = 0.0f;
			m_previewStateDuration = ClampFloat(attack.activeSec, 0.05f, 10.0f);
			if (BossAttackScript::NormalizeMovePreset(attack.movePreset) == BossAttackScript::MoveWarpBehindPlayer)
			{
				m_previewBossPos = m_previewBossTargetPos;
			}
			if (attack.visual.enabled)
			{
				Texture* texture = GetTextureForPath(attack.visual.texturePath);
				for (const PreviewZone& zone : m_previewZones)
				{
					PreviewVisual visual;
					visual.pos = zone.center;
					visual.size = attack.visual.size;
					visual.timer = ClampFloat(attack.visual.travelSec, 0.05f, 10.0f);
					visual.duration = visual.timer;
					visual.spawnHeight = ClampFloat(attack.visual.spawnHeight, 0.0f, 30.0f);
					visual.spinDegPerSec = ClampFloat(attack.visual.spinDegPerSec, -720.0f, 720.0f);
					visual.billboard = attack.visual.billboard;
					visual.texture = texture;
					m_previewVisuals.push_back(visual);
				}
			}
			UpdatePreviewHit();
		}
	}
	else if (m_previewState == PreviewActive)
	{
		const float activeSec = (attack.activeSec > 0.05f) ? attack.activeSec : 0.05f;
		const float rate = Clamp01(m_previewStateTimer / activeSec);
		switch (BossAttackScript::NormalizeMovePreset(attack.movePreset))
		{
		case BossAttackScript::MoveForward:
		case BossAttackScript::MoveChargePlayer:
		case BossAttackScript::MoveRetreat:
		case BossAttackScript::MoveToAttackOrigin:
			m_previewBossPos = {
				m_previewBossStartPos.x + (m_previewBossTargetPos.x - m_previewBossStartPos.x) * rate,
				0.0f,
				m_previewBossStartPos.z + (m_previewBossTargetPos.z - m_previewBossStartPos.z) * rate
			};
			break;
		default:
			break;
		}
		if (m_previewStateTimer >= m_previewStateDuration)
		{
			m_previewState = PreviewCooldown;
			m_previewStateTimer = 0.0f;
			m_previewStateDuration = (attack.cooldownSec > 0.001f) ? attack.cooldownSec : 0.001f;
		}
	}
	else if (m_previewState == PreviewCooldown)
	{
		if (m_previewStateTimer >= m_previewStateDuration)
		{
			--m_previewRepeatRemaining;
			if (m_previewRepeatRemaining > 0)
			{
				BeginPreviewAttack(m_previewAttackIndex);
			}
			else
			{
				m_previewPlaying = false;
				m_previewState = PreviewIdle;
				m_previewZones.clear();
			}
		}
	}
}

void SceneBossEditor::Update()
{
	if (IsMenuBackTrigger())
	{
		SceneManager::ChangeScene(SceneManager::SCENE_TITLE);
		return;
	}
	if (m_pCamera) m_pCamera->Update();
	ClampSelection();
	const float dt = 1.0f / 60.0f;
	UpdatePreviewPlayer(dt);
	UpdatePreviewPlayback(dt);
}

void SceneBossEditor::DrawReferenceFloor() const
{
	if (m_pFloorTexture)
	{
		DrawGroundSprite(m_pFloorTexture, { 0.0f, 0.0f, 0.0f }, { kStageSize, kStageSize }, { 0.55f, 0.55f, 0.55f, 0.45f });
	}
}

void SceneBossEditor::DrawPreviewScene() const
{
	if (!m_pCamera) return;
	Sprite::SetView(m_pCamera->GetViewMatrix());
	Sprite::SetProjection(m_pCamera->GetProjectionMatrix());
	DrawReferenceFloor();

	const BossAttackScript::Profile* profile = GetSelectedProfile();
	const float displayFacingYawDeg = m_previewPlaying
		? m_previewFacingYawDeg
		: ComputeFacingYawDeg(m_previewBossPos, m_previewPlayerPos, m_previewFacingYawDeg);
	const float bossScale = profile ? ClampFloat(profile->sizeScale, 0.5f, 3.0f) : 1.0f;
	const DirectX::XMFLOAT2 bossMarkerSize = { 1.8f * bossScale, 1.8f * bossScale };
	const DirectX::XMFLOAT2 playerMarkerSize = { m_previewPlayerSize.x, m_previewPlayerSize.z };
	const DirectX::XMFLOAT4 bossColor = { 0.95f, 0.35f, 0.35f, 0.95f };
	const DirectX::XMFLOAT4 playerColor = (m_previewPlayerFlashTimer > 0.0f)
		? DirectX::XMFLOAT4{ 1.0f, 0.20f, 0.20f, 1.0f }
		: DirectX::XMFLOAT4{ 0.25f, 0.90f, 0.35f, 0.95f };

	DrawGroundSprite(m_pBossTexture ? m_pBossTexture : m_pMarkerTexture, { m_previewBossPos.x, 0.02f, m_previewBossPos.z }, bossMarkerSize, bossColor, displayFacingYawDeg);
	DrawGroundSprite(m_pPlayerTexture ? m_pPlayerTexture : m_pMarkerTexture, { m_previewPlayerPos.x, 0.03f, m_previewPlayerPos.z }, playerMarkerSize, playerColor);

	for (const PreviewVisual& visual : m_previewVisuals)
	{
		if (!visual.texture) continue;
		const float rate = 1.0f - Clamp01(visual.timer / ((visual.duration > 0.05f) ? visual.duration : 0.05f));
		const DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 0.25f + 0.75f * (1.0f - rate) };
		if (visual.billboard)
		{
			DrawBillboardSprite(visual.texture, m_pCamera, { visual.pos.x, visual.spawnHeight * (1.0f - rate), visual.pos.z }, visual.size, color);
		}
		else
		{
			DrawGroundSprite(visual.texture, { visual.pos.x, 0.02f, visual.pos.z }, visual.size, color, visual.angleDeg);
		}
	}
	PrepareLineDraw(m_pCamera);
	AddRectOutline({ 0.0f, 0.01f, 0.0f }, { kStageSize, 0.0f, kStageSize }, 0.0f, { 0.35f, 0.55f, 0.90f, 1.0f });
	for (const PreviewZone& zone : m_previewZones)
	{
		const DirectX::XMFLOAT4 color = (m_previewState == PreviewTelegraph) ? DirectX::XMFLOAT4{ 1.0f, 0.35f, 0.20f, 1.0f } : DirectX::XMFLOAT4{ 1.0f, 0.92f, 0.25f, 1.0f };
		if (BossAttackScript::NormalizeColliderShape(zone.shape) == BossAttackScript::ColliderShapeCircle)
		{
			const float radius = zone.size.x * 0.5f;
			if (zone.hasPath)
			{
				AddCapsuleOutline(zone.startPos, zone.endPos, radius, color);
			}
			else
			{
				AddCircleOutline(zone.center, radius, color);
			}
		}
		else
		{
			if (zone.hasPath)
			{
				AddStripOutline(zone.startPos, zone.endPos, zone.size.x, color);
			}
			else
			{
				AddRectOutline(zone.center, zone.size, zone.yawDeg, color);
			}
		}
		if (zone.hasPath)
		{
			Geometory::AddLine(zone.startPos, zone.endPos, { 0.30f, 0.95f, 1.0f, 1.0f });
		}
	}
	if (profile && m_selectedCollider >= 0 && m_selectedCollider < static_cast<int>(profile->colliders.size()))
	{
		const BossAttackScript::Collider& selectedCollider = profile->colliders[m_selectedCollider];
		if (selectedCollider.enabled)
		{
			PreviewZone editZone = BuildPreviewZoneFromCollider(
				selectedCollider,
				{ m_previewBossPos.x, 0.0f, m_previewBossPos.z },
				m_previewPlayerPos,
				displayFacingYawDeg);
			editZone.center.y = std::max(editZone.center.y, kZoneHeight);
			editZone.size.y = std::max(editZone.size.y, kZoneHeight);

			const float baseYawDeg = (selectedCollider.startMode == BossAttackScript::ColliderStartCurrent)
				? displayFacingYawDeg
				: 0.0f;
			const float boxYawDeg = selectedCollider.useEndPosition ? editZone.yawDeg : baseYawDeg;
			DirectX::XMFLOAT3 startBoxSize = selectedCollider.startSize;
			startBoxSize.y = std::max(startBoxSize.y, kZoneHeight);
			const int selectedShape = BossAttackScript::NormalizeColliderShape(selectedCollider.shape);

			if (selectedShape == BossAttackScript::ColliderShapeCircle)
			{
				if (editZone.hasPath)
				{
					AddCapsuleOutline(editZone.startPos, editZone.endPos, editZone.size.x * 0.5f, { 0.25f, 0.95f, 1.0f, 1.0f });
				}
				else
				{
					AddCircleOutline(editZone.center, editZone.size.x * 0.5f, { 0.25f, 0.95f, 1.0f, 1.0f });
				}
			}
			else
			{
				if (editZone.hasPath)
				{
					AddStripOutline(editZone.startPos, editZone.endPos, editZone.size.x, { 0.25f, 0.95f, 1.0f, 1.0f });
				}
				else
				{
					AddRectOutline(editZone.center, editZone.size, editZone.yawDeg, { 0.25f, 0.95f, 1.0f, 1.0f });
				}
			}
			if (selectedShape == BossAttackScript::ColliderShapeCircle)
			{
				AddCircleOutline(editZone.startPos, startBoxSize.x * 0.5f, { 0.35f, 1.0f, 0.45f, 1.0f });
			}
			else
			{
				AddRectOutline(editZone.startPos, startBoxSize, boxYawDeg, { 0.35f, 1.0f, 0.45f, 1.0f });
			}

			if (selectedCollider.useEndPosition)
			{
				DirectX::XMFLOAT3 endBoxSize = selectedCollider.endSize;
				endBoxSize.y = std::max(endBoxSize.y, kZoneHeight);
				if (selectedShape == BossAttackScript::ColliderShapeCircle)
				{
					AddCircleOutline(editZone.endPos, endBoxSize.x * 0.5f, { 1.0f, 0.65f, 0.20f, 1.0f });
				}
				else
				{
					AddRectOutline(editZone.endPos, endBoxSize, editZone.yawDeg, { 1.0f, 0.65f, 0.20f, 1.0f });
				}
				Geometory::AddLine(editZone.startPos, editZone.endPos, { 0.25f, 0.95f, 1.0f, 1.0f });
			}
		}
	}
	Geometory::DrawLines();
}

void SceneBossEditor::Draw()
{
	DrawPreviewScene();
	DrawDebugWindow();
}

void SceneBossEditor::DrawDebugWindow()
{
	ImGui::SetNextWindowPos(ImVec2(24.0f, 24.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(620.0f, 860.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(SelectLabel(m_useJapaneseLabels, "Boss Attack Editor", u8"ボス攻撃エディタ")))
	{
		ImGui::End();
		return;
	}

	int languageSelection = m_useJapaneseLabels ? 1 : 0;
	const char* languageItems[] = { "English", u8"日本語" };
	if (ImGui::Combo(SelectLabel(m_useJapaneseLabels, "Language", u8"言語"), &languageSelection, languageItems, IM_ARRAYSIZE(languageItems)))
	{
		m_useJapaneseLabels = (languageSelection == 1);
	}

	ImGui::InputText(SelectLabel(m_useJapaneseLabels, "Path", u8"パス"), m_pathBuffer, IM_ARRAYSIZE(m_pathBuffer));
	ImGui::TextDisabled("%s", SelectLabel(m_useJapaneseLabels, "Use an Assets/... relative path.", u8"Assets/... の相対パスを使ってください。"));
	if (ImGui::Button(SelectLabel(m_useJapaneseLabels, "Load", u8"読み込み")))
	{
		BossAttackScript::Load(m_database, m_pathBuffer);
		ClampSelection();
		ResetPreview(true);
	}
	ImGui::SameLine();
	if (ImGui::Button(SelectLabel(m_useJapaneseLabels, "Save", u8"保存")))
	{
		BossAttackScript::Save(m_database, m_pathBuffer);
	}
	ImGui::SameLine();
	if (ImGui::Button(SelectLabel(m_useJapaneseLabels, "Reset", u8"リセット")))
	{
		m_database = BossAttackScript::MakeDefaultDatabase();
		ClampSelection();
		ResetPreview(true);
	}

	int profileType = m_selectedProfile;
	const char* profileItemsEn[BossAttackScript::ProfileTypeCount] = {};
	const char* profileItemsJp[BossAttackScript::ProfileTypeCount] = {};
	for (int i = 0; i < BossAttackScript::ProfileTypeCount; ++i)
	{
		profileItemsEn[i] = BossAttackScript::GetProfileName(i);
		profileItemsJp[i] = BossAttackScript::GetProfileNameJp(i);
	}
	if (ImGui::Combo(SelectLabel(m_useJapaneseLabels, "Boss Profile", u8"ボス種別"), &profileType, m_useJapaneseLabels ? profileItemsJp : profileItemsEn, BossAttackScript::ProfileTypeCount))
	{
		m_selectedProfile = profileType;
		ClampSelection();
		ResetPreview(true);
	}

	BossAttackScript::Profile* profile = GetSelectedProfile();
	if (!profile)
	{
		ImGui::End();
		return;
	}

	char displayName[128]{};
	strncpy_s(displayName, sizeof(displayName), profile->displayName.c_str(), _TRUNCATE);
	if (ImGui::InputText(SelectLabel(m_useJapaneseLabels, "Display Name", u8"表示名"), displayName, IM_ARRAYSIZE(displayName)))
	{
		profile->displayName = displayName;
	}
	ImGui::DragFloat(SelectLabel(m_useJapaneseLabels, "HP Scale", u8"HP倍率"), &profile->hpScale, 0.01f, 0.10f, 8.0f, "%.2f");
	ImGui::DragFloat(SelectLabel(m_useJapaneseLabels, "Guard Scale", u8"ガード倍率"), &profile->guardScale, 0.01f, 0.10f, 8.0f, "%.2f");
	ImGui::DragFloat(SelectLabel(m_useJapaneseLabels, "Size Scale", u8"サイズ倍率"), &profile->sizeScale, 0.01f, 0.20f, 3.0f, "%.2f");
	ImGui::DragFloat(SelectLabel(m_useJapaneseLabels, "Cooldown Scale", u8"CT倍率"), &profile->cooldownScale, 0.01f, 0.10f, 4.0f, "%.2f");
	ImGui::DragFloat(SelectLabel(m_useJapaneseLabels, "Telegraph Scale", u8"予兆倍率"), &profile->telegraphScale, 0.01f, 0.10f, 4.0f, "%.2f");
	ImGui::DragFloat(SelectLabel(m_useJapaneseLabels, "Damage Scale", u8"ダメージ倍率"), &profile->damageScale, 0.01f, 0.10f, 8.0f, "%.2f");
	ImGui::Checkbox(SelectLabel(m_useJapaneseLabels, "Starts Special", u8"初手特殊"), &profile->startsSpecial);
	ImGui::SameLine();
	ImGui::Checkbox(SelectLabel(m_useJapaneseLabels, "Loop", u8"ループ"), &profile->loops);

	if (ImGui::CollapsingHeader(SelectLabel(m_useJapaneseLabels, "Colliders", u8"当たり判定"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::BeginListBox("##collider_list", ImVec2(-FLT_MIN, 140.0f)))
		{
			for (int i = 0; i < static_cast<int>(profile->colliders.size()); ++i)
			{
				const BossAttackScript::Collider& collider = profile->colliders[i];
				char label[128]{};
				sprintf_s(label, sizeof(label), "%02d : %s", collider.id, collider.name.c_str());
				if (ImGui::Selectable(label, m_selectedCollider == i)) m_selectedCollider = i;
			}
			ImGui::EndListBox();
		}
		if (ImGui::Button(SelectLabel(m_useJapaneseLabels, "Add Collider", u8"当たり判定を追加")))
		{
			int nextId = 1;
			for (const auto& collider : profile->colliders) nextId = std::max(nextId, collider.id + 1);
			BossAttackScript::Collider collider;
			collider.id = nextId;
			collider.name = "Collider";
			profile->colliders.push_back(collider);
			m_selectedCollider = static_cast<int>(profile->colliders.size()) - 1;
		}
		ImGui::SameLine();
		if (ImGui::Button(SelectLabel(m_useJapaneseLabels, "Delete Collider", u8"当たり判定を削除")) && profile->colliders.size() > 1)
		{
			const int deletedId = profile->colliders[m_selectedCollider].id;
			profile->colliders.erase(profile->colliders.begin() + m_selectedCollider);
			for (BossAttackScript::Attack& attack : profile->attacks)
			{
				attack.colliderIds.erase(std::remove(attack.colliderIds.begin(), attack.colliderIds.end(), deletedId), attack.colliderIds.end());
			}
			ClampSelection();
		}
		if (m_selectedCollider >= 0 && m_selectedCollider < static_cast<int>(profile->colliders.size()))
		{
			BossAttackScript::Collider& collider = profile->colliders[m_selectedCollider];
			ImGui::Checkbox(SelectLabel(m_useJapaneseLabels, "Enabled##col", u8"有効##col"), &collider.enabled);
			ImGui::InputInt(SelectLabel(m_useJapaneseLabels, "ID", u8"ID"), &collider.id);
			char name[128]{};
			strncpy_s(name, sizeof(name), collider.name.c_str(), _TRUNCATE);
			if (ImGui::InputText(SelectLabel(m_useJapaneseLabels, "Name", u8"名前"), name, IM_ARRAYSIZE(name))) collider.name = name;
			int colliderShape = BossAttackScript::NormalizeColliderShape(collider.shape);
			const char* colliderShapeItemsEn[BossAttackScript::ColliderShapeCount] = {};
			const char* colliderShapeItemsJp[BossAttackScript::ColliderShapeCount] = {};
			for (int i = 0; i < BossAttackScript::ColliderShapeCount; ++i)
			{
				colliderShapeItemsEn[i] = BossAttackScript::GetColliderShapeName(i);
				colliderShapeItemsJp[i] = BossAttackScript::GetColliderShapeNameJp(i);
			}
			if (ImGui::Combo(
				SelectLabel(m_useJapaneseLabels, "Shape", u8"形状"),
				&colliderShape,
				m_useJapaneseLabels ? colliderShapeItemsJp : colliderShapeItemsEn,
				BossAttackScript::ColliderShapeCount))
			{
				collider.shape = colliderShape;
			}
			const bool isCircleCollider =
				(BossAttackScript::NormalizeColliderShape(collider.shape) == BossAttackScript::ColliderShapeCircle);

			int startMode = BossAttackScript::NormalizeColliderStartMode(collider.startMode);
			const char* startModeItemsEn[BossAttackScript::ColliderStartModeCount] = {};
			const char* startModeItemsJp[BossAttackScript::ColliderStartModeCount] = {};
			for (int i = 0; i < BossAttackScript::ColliderStartModeCount; ++i)
			{
				startModeItemsEn[i] = BossAttackScript::GetColliderStartModeName(i);
				startModeItemsJp[i] = BossAttackScript::GetColliderStartModeNameJp(i);
			}
			if (ImGui::Combo(
				SelectLabel(m_useJapaneseLabels, "Start Mode", u8"開始方法"),
				&startMode,
				m_useJapaneseLabels ? startModeItemsJp : startModeItemsEn,
				BossAttackScript::ColliderStartModeCount))
			{
				collider.startMode = startMode;
			}

			ImGui::DragFloat3(
				SelectLabel(m_useJapaneseLabels, "Start Position", u8"開始位置"),
				&collider.startPos.x,
				0.05f,
				-20.0f,
				20.0f,
				"%.2f");
			const int normalizedStartMode = BossAttackScript::NormalizeColliderStartMode(collider.startMode);
			if (normalizedStartMode == BossAttackScript::ColliderStartPlayerAreaRandom)
			{
				ImGui::DragFloat(
					SelectLabel(m_useJapaneseLabels, "Start Random Radius", u8"開始ランダム半径"),
					&collider.startRandomRadius,
					0.05f,
					0.0f,
					20.0f,
					"%.2f");
			}
			ImGui::Checkbox(
				SelectLabel(m_useJapaneseLabels, "Use End Position", u8"終了位置を使う"),
				&collider.useEndPosition);
			if (collider.useEndPosition)
			{
				ImGui::DragFloat2(
					SelectLabel(
						m_useJapaneseLabels,
						isCircleCollider ? "Start Diameter / Height" : "Start Thickness / Height",
						isCircleCollider ? u8"開始の直径 / 高さ" : u8"開始の太さ / 高さ"),
					&collider.startSize.x,
					0.05f,
					0.05f,
					20.0f,
					"%.2f");
				collider.startSize.z = collider.startSize.x;
			}
			else
			{
				if (isCircleCollider)
				{
					ImGui::DragFloat2(
						SelectLabel(m_useJapaneseLabels, "Start Diameter / Height", u8"開始の直径 / 高さ"),
						&collider.startSize.x,
						0.05f,
						0.05f,
						20.0f,
						"%.2f");
					collider.startSize.z = collider.startSize.x;
				}
				else
				{
					ImGui::DragFloat3(
						SelectLabel(m_useJapaneseLabels, "Start Size", u8"開始サイズ"),
						&collider.startSize.x,
						0.05f,
						0.05f,
						20.0f,
						"%.2f");
				}
			}

			int endMode = BossAttackScript::NormalizeColliderEndMode(collider.endMode);
			const char* endModeItemsEn[BossAttackScript::ColliderEndModeCount] = {};
			const char* endModeItemsJp[BossAttackScript::ColliderEndModeCount] = {};
			for (int i = 0; i < BossAttackScript::ColliderEndModeCount; ++i)
			{
				endModeItemsEn[i] = BossAttackScript::GetColliderEndModeName(i);
				endModeItemsJp[i] = BossAttackScript::GetColliderEndModeNameJp(i);
			}
			if (collider.useEndPosition &&
				ImGui::Combo(
					SelectLabel(m_useJapaneseLabels, "End Mode", u8"終了位置の基準"),
					&endMode,
					m_useJapaneseLabels ? endModeItemsJp : endModeItemsEn,
					BossAttackScript::ColliderEndModeCount))
			{
				collider.endMode = endMode;
			}

			const bool endUsesCurrent = (BossAttackScript::NormalizeColliderEndMode(collider.endMode) == BossAttackScript::ColliderEndCurrentRelative);
			const bool endUsesPlayer = (BossAttackScript::NormalizeColliderEndMode(collider.endMode) == BossAttackScript::ColliderEndPlayer);
			const bool endUsesPlayerArea = (BossAttackScript::NormalizeColliderEndMode(collider.endMode) == BossAttackScript::ColliderEndPlayerAreaRandom);
			const char* endPosLabel = endUsesCurrent
				? SelectLabel(m_useJapaneseLabels, "End Position (From Current)", u8"終了位置(現在地から)")
				: endUsesPlayer
				? SelectLabel(m_useJapaneseLabels, "End Position (Player)", u8"終了位置(プレイヤー位置)")
				: endUsesPlayerArea
				? SelectLabel(m_useJapaneseLabels, "End Position (Player Area)", u8"終了位置(プレイヤー周辺)")
				: SelectLabel(m_useJapaneseLabels, "End Position", u8"終了位置");
			if (collider.useEndPosition)
			{
				if (endUsesPlayer || endUsesPlayerArea)
				{
					ImGui::BeginDisabled();
					ImGui::DragFloat3(
						endPosLabel,
						&collider.endPos.x,
						0.05f,
						-20.0f,
						20.0f,
						"%.2f");
					ImGui::EndDisabled();
					if (endUsesPlayerArea)
					{
						ImGui::DragFloat(
							SelectLabel(m_useJapaneseLabels, "End Random Radius", u8"終了ランダム半径"),
							&collider.endRandomRadius,
							0.05f,
							0.0f,
							20.0f,
							"%.2f");
					}
				}
				else
				{
					ImGui::DragFloat3(
						endPosLabel,
						&collider.endPos.x,
						0.05f,
						-20.0f,
						20.0f,
						"%.2f");
				}
				ImGui::DragFloat2(
					SelectLabel(
						m_useJapaneseLabels,
						isCircleCollider ? "End Diameter / Height" : "End Thickness / Height",
						isCircleCollider ? u8"終了の直径 / 高さ" : u8"終了の太さ / 高さ"),
					&collider.endSize.x,
					0.05f,
					0.05f,
					20.0f,
					"%.2f");
				collider.endSize.z = collider.endSize.x;
			}
			else
			{
				ImGui::BeginDisabled();
				ImGui::DragFloat3(
					endPosLabel,
					&collider.endPos.x,
					0.05f,
					-20.0f,
					20.0f,
					"%.2f");
				if (isCircleCollider)
				{
					ImGui::DragFloat2(
						SelectLabel(m_useJapaneseLabels, "End Diameter / Height", u8"終了の直径 / 高さ"),
						&collider.endSize.x,
						0.05f,
						0.05f,
						20.0f,
						"%.2f");
				}
				else
				{
					ImGui::DragFloat3(
						SelectLabel(m_useJapaneseLabels, "End Size", u8"終了サイズ"),
						&collider.endSize.x,
						0.05f,
						0.05f,
						20.0f,
						"%.2f");
				}
				ImGui::EndDisabled();
			}
			if (collider.startMode == BossAttackScript::ColliderStartCurrent)
			{
				ImGui::TextDisabled("%s", SelectLabel(
					m_useJapaneseLabels,
					"Start position is treated as an offset from the boss current position.",
					u8"開始位置はボスの現在地からの相対位置として扱います。"));
			}
			if (collider.useEndPosition && endUsesCurrent)
			{
				ImGui::TextDisabled("%s", SelectLabel(
					m_useJapaneseLabels,
					"End position is treated as an offset from the boss current position.",
					u8"終了位置はボスの現在地からの相対位置として扱います。"));
			}
			if (collider.useEndPosition && endUsesPlayer)
			{
				ImGui::TextDisabled("%s", SelectLabel(
					m_useJapaneseLabels,
					"End position uses the current player position.",
					u8"終了位置は現在のプレイヤー位置を使います。"));
			}
			if (collider.useEndPosition && endUsesPlayerArea)
			{
				ImGui::TextDisabled("%s", SelectLabel(
					m_useJapaneseLabels,
					"End position uses a random point around the player.",
					u8"終了位置はプレイヤー周辺のランダム位置を使います。"));
			}
			if (collider.useEndPosition)
			{
				ImGui::TextDisabled("%s", SelectLabel(
					m_useJapaneseLabels,
					isCircleCollider
						? "When an end position is enabled, the collider becomes a capsule sweep from start to end. Diameter and height use the larger start/end values."
						: "When an end position is enabled, the hitbox becomes a straight strip from start to end. Thickness and height come from the larger start/end values.",
					isCircleCollider
						? u8"終了位置を使うと、当たり判定は始点から終点までのカプセル状スイープになります。直径と高さは開始/終了の大きい方を使います。"
						: u8"終了位置を使うと、当たり判定は始点から終点までの直線帯になります。太さと高さは開始/終了の大きい方を使います。"));
			}
			if (!collider.useEndPosition)
			{
				collider.endPos = collider.startPos;
				collider.endSize = collider.startSize;
			}
			if (isCircleCollider)
			{
				collider.startSize.z = collider.startSize.x;
				collider.endSize.z = collider.endSize.x;
			}
		}
	}

	if (ImGui::CollapsingHeader(SelectLabel(m_useJapaneseLabels, "Attacks", u8"攻撃"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::BeginListBox("##attack_list", ImVec2(-FLT_MIN, 160.0f)))
		{
			for (int i = 0; i < static_cast<int>(profile->attacks.size()); ++i)
			{
				const BossAttackScript::Attack& attack = profile->attacks[i];
				char label[160]{};
				sprintf_s(label, sizeof(label), "%02d : %s %s", i, attack.name.c_str(), attack.enabled ? "" : "(Off)");
				if (ImGui::Selectable(label, m_selectedAttack == i)) m_selectedAttack = i;
			}
			ImGui::EndListBox();
		}
		if (ImGui::Button(SelectLabel(m_useJapaneseLabels, "Add Attack", u8"攻撃を追加")))
		{
			BossAttackScript::Attack attack;
			attack.name = "Attack";
			if (!profile->colliders.empty()) attack.colliderIds.push_back(profile->colliders.front().id);
			profile->attacks.push_back(attack);
			m_selectedAttack = static_cast<int>(profile->attacks.size()) - 1;
		}
		ImGui::SameLine();
		if (ImGui::Button(SelectLabel(m_useJapaneseLabels, "Delete Attack", u8"攻撃を削除")) && profile->attacks.size() > 1)
		{
			profile->attacks.erase(profile->attacks.begin() + m_selectedAttack);
			ClampSelection();
		}
		if (m_selectedAttack >= 0 && m_selectedAttack < static_cast<int>(profile->attacks.size()))
		{
			BossAttackScript::Attack& attack = profile->attacks[m_selectedAttack];
			ImGui::Checkbox(SelectLabel(m_useJapaneseLabels, "Enabled##atk", u8"有効##atk"), &attack.enabled);
			char attackName[128]{};
			strncpy_s(attackName, sizeof(attackName), attack.name.c_str(), _TRUNCATE);
			if (ImGui::InputText(SelectLabel(m_useJapaneseLabels, "Attack Name", u8"攻撃名"), attackName, IM_ARRAYSIZE(attackName))) attack.name = attackName;
			int deliveryMode = BossAttackScript::NormalizeAttackDeliveryMode(attack.deliveryMode);
			int attackOriginMode = (deliveryMode == BossAttackScript::AttackDeliveryBossSelf) ? 0 : 1;
			const char* attackOriginItemsEn[] = { "Boss Self", "Remote" };
			const char* attackOriginItemsJp[] = { u8"ボス自身", u8"遠隔" };
			if (ImGui::Combo(
				SelectLabel(m_useJapaneseLabels, "Attack Origin", u8"攻撃元"),
				&attackOriginMode,
				m_useJapaneseLabels ? attackOriginItemsJp : attackOriginItemsEn,
				IM_ARRAYSIZE(attackOriginItemsEn)))
			{
				if (attackOriginMode == 0)
				{
					attack.deliveryMode = BossAttackScript::AttackDeliveryBossSelf;
				}
				else if (deliveryMode == BossAttackScript::AttackDeliveryBossSelf)
				{
					attack.deliveryMode = BossAttackScript::AttackDeliveryRemoteFalling;
				}
			}
			deliveryMode = BossAttackScript::NormalizeAttackDeliveryMode(attack.deliveryMode);
			if (deliveryMode != BossAttackScript::AttackDeliveryBossSelf)
			{
				int remoteType = (deliveryMode == BossAttackScript::AttackDeliveryRemoteGround) ? 1 : 0;
				const char* remoteTypeItemsEn[] = { "Falling", "Ground" };
				const char* remoteTypeItemsJp[] = { u8"落下", u8"地上" };
				if (ImGui::Combo(
					SelectLabel(m_useJapaneseLabels, "Remote Type", u8"遠隔種類"),
					&remoteType,
					m_useJapaneseLabels ? remoteTypeItemsJp : remoteTypeItemsEn,
					IM_ARRAYSIZE(remoteTypeItemsEn)))
				{
					attack.deliveryMode = (remoteType == 0)
						? BossAttackScript::AttackDeliveryRemoteFalling
						: BossAttackScript::AttackDeliveryRemoteGround;
				}
			}
			ImGui::DragFloat(SelectLabel(m_useJapaneseLabels, "Telegraph", u8"予兆時間"), &attack.telegraphSec, 0.01f, 0.05f, 20.0f, "%.2f");
			ImGui::DragFloat(SelectLabel(m_useJapaneseLabels, "Active", u8"攻撃時間"), &attack.activeSec, 0.01f, 0.05f, 10.0f, "%.2f");
			ImGui::DragFloat(SelectLabel(m_useJapaneseLabels, "Cooldown", u8"次まで"), &attack.cooldownSec, 0.01f, 0.0f, 20.0f, "%.2f");
			ImGui::DragFloat(SelectLabel(m_useJapaneseLabels, "Damage Scale", u8"ダメージ倍率"), &attack.damageScale, 0.01f, 0.0f, 20.0f, "%.2f");
			ImGui::DragInt(SelectLabel(m_useJapaneseLabels, "Repeat Count", u8"繰返回数"), &attack.repeatCount, 0.1f, 1, 64);
			ImGui::DragFloat(SelectLabel(m_useJapaneseLabels, "Repeat Interval", u8"繰返間隔"), &attack.repeatIntervalSec, 0.01f, 0.0f, 10.0f, "%.2f");
			const bool isBossSelf = (BossAttackScript::NormalizeAttackDeliveryMode(attack.deliveryMode) == BossAttackScript::AttackDeliveryBossSelf);
			const bool isRemoteFalling = (BossAttackScript::NormalizeAttackDeliveryMode(attack.deliveryMode) == BossAttackScript::AttackDeliveryRemoteFalling);
			const bool isRemoteGround = (BossAttackScript::NormalizeAttackDeliveryMode(attack.deliveryMode) == BossAttackScript::AttackDeliveryRemoteGround);
			int movePreset = BossAttackScript::NormalizeMovePreset(attack.movePreset);
			const char* moveItemsEn[BossAttackScript::MovePresetCount] = {};
			const char* moveItemsJp[BossAttackScript::MovePresetCount] = {};
			for (int i = 0; i < BossAttackScript::MovePresetCount; ++i) { moveItemsEn[i] = BossAttackScript::GetMovePresetName(i); moveItemsJp[i] = BossAttackScript::GetMovePresetNameJp(i); }
			if (!isBossSelf) ImGui::BeginDisabled();
			if (ImGui::Combo(SelectLabel(m_useJapaneseLabels, "Boss Move", u8"ボス移動"), &movePreset, m_useJapaneseLabels ? moveItemsJp : moveItemsEn, BossAttackScript::MovePresetCount)) attack.movePreset = movePreset;
			if (!isBossSelf) ImGui::EndDisabled();
			int spawnMode = BossAttackScript::NormalizeSpawnMode(attack.spawnMode);
			const char* spawnItemsEn[BossAttackScript::SpawnModeCount] = {};
			const char* spawnItemsJp[BossAttackScript::SpawnModeCount] = {};
			for (int i = 0; i < BossAttackScript::SpawnModeCount; ++i) { spawnItemsEn[i] = BossAttackScript::GetSpawnModeName(i); spawnItemsJp[i] = BossAttackScript::GetSpawnModeNameJp(i); }
			if (ImGui::Combo(
				SelectLabel(m_useJapaneseLabels,
					isRemoteFalling ? "Targeting" : (isRemoteGround ? "Ground Spawn" : "Spawn Mode"),
					isRemoteFalling ? u8"落下先" : (isRemoteGround ? u8"地上攻撃の発生基準" : u8"発生方法")),
				&spawnMode,
				m_useJapaneseLabels ? spawnItemsJp : spawnItemsEn,
				BossAttackScript::SpawnModeCount)) attack.spawnMode = spawnMode;
			if (!isBossSelf) ImGui::BeginDisabled();
			ImGui::DragFloat(SelectLabel(m_useJapaneseLabels, "Move Distance", u8"移動距離"), &attack.moveDistance, 0.05f, 0.0f, 20.0f, "%.2f");
			ImGui::DragFloat(SelectLabel(m_useJapaneseLabels, "Move Speed", u8"移動速度"), &attack.moveSpeed, 0.05f, 0.0f, 30.0f, "%.2f");
			if (!isBossSelf) ImGui::EndDisabled();
			ImGui::DragInt(
				SelectLabel(m_useJapaneseLabels,
					isRemoteFalling ? "Drop Count" : "Spawn Count",
					isRemoteFalling ? u8"同時落下数" : u8"発生数"),
				&attack.spawnCount,
				0.1f,
				1,
				64);
			if (spawnMode == BossAttackScript::SpawnArenaRandom || spawnMode == BossAttackScript::SpawnPlayerAreaRandom)
			{
				ImGui::DragFloat(SelectLabel(m_useJapaneseLabels, "Random Radius", u8"ランダム半径"), &attack.randomRadius, 0.05f, 0.0f, 20.0f, "%.2f");
			}
			if (isRemoteFalling)
			{
				ImGui::TextDisabled("%s",
					SelectLabel(m_useJapaneseLabels,
						"Remote Falling uses collider start settings as the landing position reference.",
						u8"遠隔(落下)は、割り当てたコライダーの開始位置を落下地点の基準として使います。"));
			}
			else if (isRemoteGround)
			{
				ImGui::TextDisabled("%s",
					SelectLabel(m_useJapaneseLabels,
						"Remote Ground can use collider start/end settings like a projected ground sweep.",
						u8"遠隔(地上)は、コライダーの開始位置と終了位置を使って地上攻撃を作れます。"));
			}
			ImGui::TextDisabled("%s", SelectLabel(m_useJapaneseLabels, "Collider Links", u8"コライダー割当"));
			for (const BossAttackScript::Collider& collider : profile->colliders)
			{
				bool used = std::find(attack.colliderIds.begin(), attack.colliderIds.end(), collider.id) != attack.colliderIds.end();
				char label[128]{};
				sprintf_s(label, sizeof(label), "%02d : %s", collider.id, collider.name.c_str());
				if (ImGui::Checkbox(label, &used))
				{
					if (used)
					{
						if (std::find(attack.colliderIds.begin(), attack.colliderIds.end(), collider.id) == attack.colliderIds.end()) attack.colliderIds.push_back(collider.id);
					}
					else
					{
						attack.colliderIds.erase(std::remove(attack.colliderIds.begin(), attack.colliderIds.end(), collider.id), attack.colliderIds.end());
					}
				}
			}
			ImGui::Checkbox(SelectLabel(m_useJapaneseLabels, "Use Visual", u8"見た目を使う"), &attack.visual.enabled);
			ImGui::SameLine();
			ImGui::Checkbox(SelectLabel(m_useJapaneseLabels, "Billboard", u8"ビルボード"), &attack.visual.billboard);
			char texturePath[260]{};
			strncpy_s(texturePath, sizeof(texturePath), attack.visual.texturePath.c_str(), _TRUNCATE);
			if (ImGui::InputText(SelectLabel(m_useJapaneseLabels, "Texture Path", u8"テクスチャパス"), texturePath, IM_ARRAYSIZE(texturePath))) attack.visual.texturePath = texturePath;
			ImGui::DragFloat2(SelectLabel(m_useJapaneseLabels, "Visual Size", u8"見た目サイズ"), &attack.visual.size.x, 0.05f, 0.05f, 20.0f, "%.2f");
			ImGui::DragFloat(SelectLabel(m_useJapaneseLabels, "Spawn Height", u8"出現高さ"), &attack.visual.spawnHeight, 0.05f, 0.0f, 30.0f, "%.2f");
			ImGui::DragFloat(SelectLabel(m_useJapaneseLabels, "Travel Sec", u8"落下時間"), &attack.visual.travelSec, 0.01f, 0.05f, 10.0f, "%.2f");
			ImGui::DragFloat(SelectLabel(m_useJapaneseLabels, "Spin", u8"回転速度"), &attack.visual.spinDegPerSec, 1.0f, -720.0f, 720.0f, "%.0f");
		}
	}

	ImGui::Separator();
	if (ImGui::Button(SelectLabel(m_useJapaneseLabels, "Play Selected", u8"選択攻撃を再生")))
	{
		m_previewPlaying = true;
		m_previewRepeatRemaining = 0;
		BeginPreviewAttack(m_selectedAttack);
	}
	ImGui::SameLine();
	if (ImGui::Button(SelectLabel(m_useJapaneseLabels, "Stop", u8"停止")))
	{
		ResetPreview(false);
	}
	ImGui::SameLine();
	if (ImGui::Button(SelectLabel(m_useJapaneseLabels, "Reset Player", u8"プレイヤー位置リセット")))
	{
		ResetPreview(true);
	}
	ImGui::Text("%s : %.2f, %.2f", SelectLabel(m_useJapaneseLabels, "Player", u8"プレイヤー"), m_previewPlayerPos.x, m_previewPlayerPos.z);
	ImGui::Text("%s : %.2f, %.2f", SelectLabel(m_useJapaneseLabels, "Boss", u8"ボス"), m_previewBossPos.x, m_previewBossPos.z);
	ImGui::Text("%s : %s",
		SelectLabel(m_useJapaneseLabels, "State", u8"状態"),
		(m_previewState == PreviewTelegraph) ? SelectLabel(m_useJapaneseLabels, "Telegraph", u8"予兆") :
		(m_previewState == PreviewActive) ? SelectLabel(m_useJapaneseLabels, "Active", u8"攻撃中") :
		(m_previewState == PreviewCooldown) ? SelectLabel(m_useJapaneseLabels, "Cooldown", u8"待機") :
		SelectLabel(m_useJapaneseLabels, "Idle", u8"停止"));
	ImGui::Separator();
	ImGui::TextUnformatted(SelectLabel(m_useJapaneseLabels, "Color Legend", u8"色の説明"));
	auto drawLegend = [&](const ImVec4& color, const char* en, const char* jp)
	{
		ImGui::TextColored(color, u8"■");
		ImGui::SameLine();
		ImGui::TextUnformatted(SelectLabel(m_useJapaneseLabels, en, jp));
	};
	drawLegend(ImVec4(0.35f, 0.55f, 0.90f, 1.0f), "Stage bounds", u8"ステージ外周");
	drawLegend(ImVec4(1.0f, 0.35f, 0.20f, 1.0f), "Telegraph zone", u8"予兆中の攻撃範囲");
	drawLegend(ImVec4(1.0f, 0.92f, 0.25f, 1.0f), "Active zone", u8"攻撃発生中の範囲");
	drawLegend(ImVec4(0.25f, 0.95f, 1.0f, 1.0f), "Selected collider / path", u8"選択中コライダー全体 / 経路");
	drawLegend(ImVec4(0.35f, 1.0f, 0.45f, 1.0f), "Start point and start size", u8"始点と開始サイズ");
	drawLegend(ImVec4(1.0f, 0.65f, 0.20f, 1.0f), "End point and end size", u8"終点と終了サイズ");
	drawLegend(ImVec4(0.95f, 0.35f, 0.35f, 0.95f), "Boss marker", u8"ボスマーカー");
	drawLegend(ImVec4(0.25f, 0.90f, 0.35f, 0.95f), "Player marker", u8"プレイヤーマーカー");
	drawLegend(ImVec4(1.0f, 0.20f, 0.20f, 1.0f), "Player hit flash", u8"プレイヤー被弾時");
	ImGui::TextDisabled("%s", SelectLabel(m_useJapaneseLabels, "Camera: RMB rotate / MMB pan / Wheel zoom", u8"カメラ: 右ドラッグ回転 / 中ドラッグ平行移動 / ホイールズーム"));
	ImGui::TextDisabled("%s", SelectLabel(m_useJapaneseLabels, "Preview Player: WASD or pad left stick", u8"プレビュープレイヤー: WASD または左スティック"));
	ImGui::End();
}

