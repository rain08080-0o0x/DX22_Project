#include "SceneGame.h"
#include "Collision.h"
#include "Defines.h"
#include "Input.h"
#include "Main.h"
#include "SceneManager.h"
#include "Sound.h"
#include "Sprite.h"
#include "Texture.h"
#include "Transfer.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace
{
    const float kBossFixedDt = 1.0f / 60.0f;

    float Clamp01(float v)
    {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    }

    float ClampRange(float v, float lo, float hi)
    {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    int ClampInt(int v, int lo, int hi)
    {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    float MaxFloat(float a, float b)
    {
        return (a > b) ? a : b;
    }

    float SafeSpan(float v, float fallback)
    {
        return (v > 0.05f) ? v : fallback;
    }

    float Random01()
    {
        const int maxRand = (RAND_MAX > 0) ? RAND_MAX : 1;
        return static_cast<float>(std::rand()) / static_cast<float>(maxRand);
    }

    float RandomRange(float lo, float hi)
    {
        return lo + (hi - lo) * Random01();
    }

    int RandomRangeInt(int lo, int hi)
    {
        if (hi <= lo) return lo;
        return lo + (std::rand() % (hi - lo + 1));
    }

    DirectX::XMFLOAT3 LerpFloat3(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, float t)
    {
        const float rate = Clamp01(t);
        return {
            a.x + (b.x - a.x) * rate,
            a.y + (b.y - a.y) * rate,
            a.z + (b.z - a.z) * rate
        };
    }

    Collision::Box MakeAabb(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& size)
    {
        Collision::Box box{};
        box.center = center;
        box.size = size;
        return box;
    }

    bool HitAabb(const Collision::Box& a, const Collision::Box& b)
    {
        return Collision::Hit(a, b).isHit;
    }

    void DrawAttackMarkerTintLocal(Texture* texture,
                                   const DirectX::XMFLOAT3& pos,
                                   const DirectX::XMFLOAT3& size,
                                   const DirectX::XMFLOAT4& color)
    {
        if (!texture) return;

        const float markerY = 0.002f;
        DirectX::XMMATRIX r = DirectX::XMMatrixRotationX(DirectX::XM_PIDIV2);
        DirectX::XMMATRIX t = DirectX::XMMatrixTranslation(pos.x, markerY, pos.z);
        DirectX::XMFLOAT4X4 world{};
        DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranspose(r * t));

        Sprite::SetWorld(world);
        Sprite::SetSize({ size.x, size.z });
        Sprite::SetOffset({ 0.0f, 0.0f });
        Sprite::SetUVPos({ 0.0f, 0.0f });
        Sprite::SetUVScale({ 1.0f, 1.0f });
        Sprite::SetColor(color);
        Sprite::SetTexture(texture);
        Sprite::Draw();
    }

    void DrawBillboardSpriteLocal(Texture* texture,
                                  Camera* camera,
                                  const DirectX::XMFLOAT3& pos,
                                  const DirectX::XMFLOAT3& size,
                                  const DirectX::XMFLOAT4& color)
    {
        if (!texture) return;
        using namespace DirectX;

        XMMATRIX billboard = XMMatrixIdentity();
        if (camera)
        {
            XMFLOAT4X4 viewFloat = camera->GetViewMatrix(false);
            XMMATRIX viewMat = XMLoadFloat4x4(&viewFloat);
            XMMATRIX invView = XMMatrixInverse(nullptr, viewMat);
            XMFLOAT4X4 invViewFloat{};
            XMStoreFloat4x4(&invViewFloat, invView);
            invViewFloat._41 = 0.0f;
            invViewFloat._42 = 0.0f;
            invViewFloat._43 = 0.0f;
            billboard = XMLoadFloat4x4(&invViewFloat);
        }

        const XMMATRIX t = billboard * XMMatrixTranslation(
            pos.x,
            pos.y + (size.y * 0.5f),
            pos.z);
        XMFLOAT4X4 world{};
        XMStoreFloat4x4(&world, XMMatrixTranspose(t));

        Sprite::SetWorld(world);
        Sprite::SetSize({ size.x, size.y });
        Sprite::SetOffset({ 0.0f, 0.0f });
        Sprite::SetUVPos({ 0.0f, 0.0f });
        Sprite::SetUVScale({ 1.0f, 1.0f });
        Sprite::SetColor(color);
        Sprite::SetTexture(texture);
        Sprite::Draw();
    }

    void DrawBossHpOverlayLocal(float hpRate, float barWidthRate, float barHeightRate)
    {
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        if (!dl) return;

        const float widthRate = ClampRange(barWidthRate, 0.20f, 0.90f);
        const float heightRate = ClampRange(barHeightRate, 0.01f, 0.20f);
        const float barW = static_cast<float>(SCREEN_WIDTH) * widthRate;
        const float barH = static_cast<float>(SCREEN_HEIGHT) * heightRate;
        const float x = (static_cast<float>(SCREEN_WIDTH) - barW) * 0.5f;
        const float y = 12.0f;
        const float padding = 4.0f;
        const float radius = 6.0f;

        const ImVec2 frameMin(x, y);
        const ImVec2 frameMax(x + barW, y + barH);
        dl->AddRectFilled(frameMin, frameMax, IM_COL32(20, 20, 20, 220), radius);
        dl->AddRect(frameMin, frameMax, IM_COL32(230, 230, 230, 210), radius, 0, 2.0f);

        const float fillRate = Clamp01(hpRate);
        const float innerW = (barW - padding * 2.0f) * fillRate;
        const ImVec2 fillMin(x + padding, y + padding);
        const ImVec2 fillMax(x + padding + innerW, y + barH - padding);
        if (innerW > 0.0f)
        {
            dl->AddRectFilled(fillMin, fillMax, IM_COL32(180, 30, 30, 220), radius * 0.6f);
        }

        dl->AddText(ImVec2(x + 8.0f, y - 18.0f), IM_COL32(255, 255, 255, 230), "BOSS");
    }

    void ReplaceTexture(Texture*& texture, const char* path, const char* label)
    {
        if (texture)
        {
            delete texture;
            texture = nullptr;
        }

        texture = new Texture();
        if (!texture || FAILED(texture->Create(path)))
        {
            MessageBox(NULL, label, "Error", MB_OK);
        }
    }

    void ReleaseTexturePtr(Texture*& texture)
    {
        if (texture)
        {
            delete texture;
            texture = nullptr;
        }
    }

    void PushAttackZone(std::vector<BossController::AttackZone>& zones,
                        const DirectX::XMFLOAT3& center,
                        const DirectX::XMFLOAT3& size,
                        const DirectX::XMFLOAT4& color,
                        float revealStart = 0.0f,
                        bool safeZone = false)
    {
        BossController::AttackZone zone;
        zone.center = center;
        zone.size = size;
        zone.color = color;
        zone.revealStart = revealStart;
        zone.safeZone = safeZone;
        zones.push_back(zone);
    }

    void SpawnRockVisual(std::vector<BossController::FallingRock>& rocks,
                         const DirectX::XMFLOAT3& center,
                         float span)
    {
        BossController::FallingRock rock;
        const float safeSpan = SafeSpan(span, 1.0f);
        rock.pos = { center.x, 0.0f, center.z };
        rock.size = { safeSpan, safeSpan, safeSpan };
        rock.duration = 0.28f;
        rock.timer = rock.duration;
        rocks.push_back(rock);
    }
}

void BossController::ResetForScene(const DirectX::XMFLOAT3& playerSize, float bossSizeAreaScale, int bossMaxHp)
{
    const float areaScale = ClampRange(bossSizeAreaScale, 4.0f, 12.0f);
    const float bossScale = std::sqrt(areaScale);
    size = {
        playerSize.x * bossScale,
        playerSize.y * bossScale,
        playerSize.z * bossScale
    };
    pos = { 0.0f, 0.0f, 0.0f };
    maxHp = ClampInt(bossMaxHp, 1, 9999);
    hp = maxHp;
    lastHitSwingId = -1;
    attackKind = AttackKindDashNarrow;
    attackPattern = AttackPatternVertical;
    attackLane.center = { 0.0f, 0.0f, 0.0f };
    attackLane.size = { 0.0f, 0.0f, 0.0f };
    attackLane.pattern = attackPattern;
    attackZones.clear();
    fallingRocks.clear();
    attackState = AttackIdle;
    attackStateTimer = 0.0f;
    attackTelegraphDuration = 0.0f;
    attackExecuteDuration = 0.0f;
    attackCooldownTimer = 0.0f;
    attackRepeatsRemaining = 0;
    attackCycleCount = 0;
    dashStartPos = { 0.0f, 0.0f, 0.0f };
    dashEndPos = { 0.0f, 0.0f, 0.0f };
    attackResolved = false;
    jumpedOut = false;
    requiresArenaReset = true;
}

void BossController::LoadTexture(const char* path)
{
    ReplaceTexture(texture, path, "Texture load failed.\nBoss texture");
}

void BossController::LoadRockTexture(const char* path)
{
    ReplaceTexture(rockTexture, path, "Texture load failed.\nBoss rock texture");
}

void BossController::ReleaseTexture()
{
    ReleaseTexturePtr(texture);
}

void BossController::ReleaseRockTexture()
{
    ReleaseTexturePtr(rockTexture);
}

void SceneGame::InitializeBossForScene()
{
    auto& tran = Transfer::GetInstance();
    m_boss.ResetForScene(tran.player.size, tran.gameplay.bossSizeAreaScale, tran.gameplay.bossMaxHp);
}

void SceneGame::LoadBossResources()
{
    m_boss.LoadTexture("Assets/Texture/Chracter/genbaneko.png");
    m_boss.LoadRockTexture("Assets/Texture/Game/rock.png");
}

void SceneGame::ReleaseBossResources()
{
    m_boss.ReleaseRockTexture();
    m_boss.ReleaseTexture();
}

bool SceneGame::UpdateBossDebugSetup(float stageSize)
{
    if (!m_isBossBattleDebug)
    {
        return false;
    }

    auto& tran = Transfer::GetInstance();
    if (m_boss.requiresArenaReset)
    {
        EnsureEnemyCount(0, stageSize);
        m_enemyProjectiles.clear();
        m_requestedEnemyCount = 0;
        m_boss.requiresArenaReset = false;
    }
    tran.gameplayDebug.bossBattleActive = 1;
    tran.gameplayDebug.bossHp = static_cast<float>(m_boss.hp);
    tran.gameplayDebug.bossMaxHp = static_cast<float>(m_boss.maxHp);
    tran.gameplay.bossHpBarWidthRate = ClampRange(tran.gameplay.bossHpBarWidthRate, 0.20f, 0.90f);
    tran.gameplay.bossHpBarHeightRate = ClampRange(tran.gameplay.bossHpBarHeightRate, 0.01f, 0.20f);
    tran.gameplay.bossSizeAreaScale = ClampRange(tran.gameplay.bossSizeAreaScale, 4.0f, 12.0f);
    tran.gameplay.bossAttackJumpOutTime = ClampRange(tran.gameplay.bossAttackJumpOutTime, 0.0f, 4.0f);
    tran.gameplay.bossAttackDashDuration = ClampRange(tran.gameplay.bossAttackDashDuration, 0.05f, 2.0f);
    tran.gameplay.bossAttackCooldown = ClampRange(tran.gameplay.bossAttackCooldown, 0.0f, 6.0f);
    tran.gameplay.bossAttackTelegraph = ClampRange(tran.gameplay.bossAttackTelegraph, 0.10f, 4.0f);
    tran.gameplay.bossAttackLanePlayerScale = ClampRange(tran.gameplay.bossAttackLanePlayerScale, 0.5f, 8.0f);
    if (tran.gameplay.bossAttackDamage < 0.0f) tran.gameplay.bossAttackDamage = 0.0f;
    tran.gameplay.bossDashNarrowTelegraph = ClampRange(tran.gameplay.bossDashNarrowTelegraph, 0.10f, 8.0f);
    tran.gameplay.bossDashWideTelegraph = ClampRange(tran.gameplay.bossDashWideTelegraph, 0.10f, 8.0f);
    tran.gameplay.bossDashWideWidthRate = ClampRange(tran.gameplay.bossDashWideWidthRate, 0.10f, 1.00f);
    tran.gameplay.bossRandomRainCount = ClampInt(tran.gameplay.bossRandomRainCount, 1, 16);
    tran.gameplay.bossRandomRainTelegraph = ClampRange(tran.gameplay.bossRandomRainTelegraph, 0.10f, 8.0f);
    if (tran.gameplay.bossRandomRainRadiusScale < 0.25f) tran.gameplay.bossRandomRainRadiusScale = 0.25f;
    tran.gameplay.bossSummonMin = ClampInt(tran.gameplay.bossSummonMin, 1, 32);
    tran.gameplay.bossSummonMax = ClampInt(tran.gameplay.bossSummonMax, 1, 32);
    if (tran.gameplay.bossSummonMax < tran.gameplay.bossSummonMin) tran.gameplay.bossSummonMax = tran.gameplay.bossSummonMin;
    tran.gameplay.bossSummonTelegraph = ClampRange(tran.gameplay.bossSummonTelegraph, 0.10f, 8.0f);
    tran.gameplay.bossTrackingDropCount = ClampInt(tran.gameplay.bossTrackingDropCount, 1, 16);
    tran.gameplay.bossTrackingDropTelegraph = ClampRange(tran.gameplay.bossTrackingDropTelegraph, 0.10f, 8.0f);
    if (tran.gameplay.bossTrackingDropRadiusScale < 0.5f) tran.gameplay.bossTrackingDropRadiusScale = 0.5f;
    tran.gameplay.bossUltimateCrossTelegraph = ClampRange(tran.gameplay.bossUltimateCrossTelegraph, 0.10f, 8.0f);
    if (tran.gameplay.bossUltimateCrossLaneScale < 0.25f) tran.gameplay.bossUltimateCrossLaneScale = 0.25f;
    tran.gameplay.bossUltimateStompCount = ClampInt(tran.gameplay.bossUltimateStompCount, 1, 16);
    tran.gameplay.bossUltimateStompTelegraph = ClampRange(tran.gameplay.bossUltimateStompTelegraph, 0.10f, 12.0f);
    if (tran.gameplay.bossUltimateStompRadiusScale < 0.5f) tran.gameplay.bossUltimateStompRadiusScale = 0.5f;
    tran.gameplay.bossUltimateFieldTelegraph = ClampRange(tran.gameplay.bossUltimateFieldTelegraph, 0.10f, 12.0f);
    if (tran.gameplay.bossUltimateFieldSafeScale < 0.5f) tran.gameplay.bossUltimateFieldSafeScale = 0.5f;
    tran.gameplay.bossMaxHp = ClampInt(tran.gameplay.bossMaxHp, 1, 9999);
    if (m_boss.maxHp <= 0)
    {
        m_boss.maxHp = tran.gameplay.bossMaxHp;
        m_boss.hp = m_boss.maxHp;
    }
    else if (m_boss.maxHp != tran.gameplay.bossMaxHp)
    {
        const float hpRate = Clamp01(static_cast<float>(m_boss.hp) / static_cast<float>(m_boss.maxHp));
        m_boss.maxHp = tran.gameplay.bossMaxHp;
        m_boss.hp = ClampInt(static_cast<int>(std::ceil(hpRate * static_cast<float>(m_boss.maxHp))), 0, m_boss.maxHp);
    }
    else if (m_boss.hp > m_boss.maxHp)
    {
        m_boss.hp = m_boss.maxHp;
    }

    const float bossScale = std::sqrt(tran.gameplay.bossSizeAreaScale);
    m_boss.size =
    {
        tran.player.size.x * bossScale,
        tran.player.size.y * bossScale,
        tran.player.size.z * bossScale
    };

    if (IsKeyTrigger(VK_RETURN))
    {
        m_boss.attackZones.clear();
        m_boss.fallingRocks.clear();
        tran.gameplayDebug.runTimerRunning = 0;
        tran.gameplayDebug.runRecordedSec = tran.gameplayDebug.runElapsedSec;
        tran.gameplayDebug.bossBattleActive = 0;
        tran.gameplayDebug.showBossResultTimer = 1;
        tran.gameplayDebug.upgradeSelectionPending = 0;
        tran.gameplayDebug.upgradeRerollRemain = 0;
        tran.roguelike.selectionPending = 0;
        tran.roguelike.rerollRemain = 0;
        if (m_pClearSe) PlaySound(m_pClearSe);
        SceneManager::ChangeResult(SceneManager::ResultType::Win);
        SceneManager::ChangeScene(SceneManager::SCENE_RESULT);
        return true;
    }

    return false;
}

bool SceneGame::UpdateBossBattle(float stageSize,
                                 int playerAttackDamage,
                                 const std::function<bool(float)>& applyPlayerDamage)
{
    if (!m_isBossBattleDebug || !m_pPlayer)
    {
        auto& tran = Transfer::GetInstance();
        tran.gameplayDebug.bossHp = 0.0f;
        tran.gameplayDebug.bossMaxHp = 0.0f;
        return false;
    }

    auto& tran = Transfer::GetInstance();
    tran.gameplayDebug.bossHp = static_cast<float>(m_boss.hp);
    tran.gameplayDebug.bossMaxHp = static_cast<float>(m_boss.maxHp);
    for (auto& rock : m_boss.fallingRocks)
    {
        if (rock.timer > 0.0f)
        {
            rock.timer -= kBossFixedDt;
            if (rock.timer < 0.0f) rock.timer = 0.0f;
        }
    }
    m_boss.fallingRocks.erase(
        std::remove_if(
            m_boss.fallingRocks.begin(),
            m_boss.fallingRocks.end(),
            [](const BossController::FallingRock& rock) { return rock.timer <= 0.0f; }),
        m_boss.fallingRocks.end());

    const float stageHalf = stageSize * 0.5f;
    const float dashSec = ClampRange(tran.gameplay.bossAttackDashDuration, 0.05f, 2.0f);
    const float jumpOutSec = ClampRange(tran.gameplay.bossAttackJumpOutTime, 0.0f, 4.0f);
    const float cooldownSec = ClampRange(tran.gameplay.bossAttackCooldown, 0.0f, 6.0f);
    const float telegraphMultiplier = ClampRange(tran.gameplay.bossAttackTelegraph, 0.10f, 4.0f);
    const float bossDamage = (tran.gameplay.bossAttackDamage < 0.0f) ? 0.0f : tran.gameplay.bossAttackDamage;
    const float lanePlayerRatio = ClampRange(tran.gameplay.bossAttackLanePlayerScale, 0.5f, 8.0f);
    const float globalLaneScale = lanePlayerRatio / 3.0f;
    const float playerWidth = SafeSpan(tran.player.size.x, 0.8f);
    const float playerDepth = SafeSpan(tran.player.size.z, playerWidth);
    const float playerSpan = MaxFloat(playerWidth, playerDepth);
    const float zoneHeight = MaxFloat(m_boss.size.y, tran.player.size.y);
    const float dashNarrowTelegraphSec = ClampRange(tran.gameplay.bossDashNarrowTelegraph, 0.10f, 8.0f) * telegraphMultiplier;
    const float dashWideTelegraphSec = ClampRange(tran.gameplay.bossDashWideTelegraph, 0.10f, 8.0f) * telegraphMultiplier;
    const float dashWideWidthRate = ClampRange(tran.gameplay.bossDashWideWidthRate, 0.10f, 1.00f);
    const int randomRainCount = ClampInt(tran.gameplay.bossRandomRainCount, 1, 16);
    const float randomRainTelegraphSec = ClampRange(tran.gameplay.bossRandomRainTelegraph, 0.10f, 8.0f) * telegraphMultiplier;
    const float randomRainRadiusScale = (tran.gameplay.bossRandomRainRadiusScale < 0.25f) ? 0.25f : tran.gameplay.bossRandomRainRadiusScale;
    const int summonMin = ClampInt(tran.gameplay.bossSummonMin, 1, 32);
    const int summonMax = ClampInt((tran.gameplay.bossSummonMax < summonMin) ? summonMin : tran.gameplay.bossSummonMax, summonMin, 32);
    const float summonTelegraphSec = ClampRange(tran.gameplay.bossSummonTelegraph, 0.10f, 8.0f) * telegraphMultiplier;
    const int trackingDropCount = ClampInt(tran.gameplay.bossTrackingDropCount, 1, 16);
    const float trackingDropTelegraphSec = ClampRange(tran.gameplay.bossTrackingDropTelegraph, 0.10f, 8.0f) * telegraphMultiplier;
    const float trackingDropRadiusScale = (tran.gameplay.bossTrackingDropRadiusScale < 0.5f) ? 0.5f : tran.gameplay.bossTrackingDropRadiusScale;
    const float ultimateCrossTelegraphSec = ClampRange(tran.gameplay.bossUltimateCrossTelegraph, 0.10f, 8.0f) * telegraphMultiplier;
    const float ultimateCrossLaneScale = (tran.gameplay.bossUltimateCrossLaneScale < 0.25f) ? 0.25f : tran.gameplay.bossUltimateCrossLaneScale;
    const int ultimateStompCount = ClampInt(tran.gameplay.bossUltimateStompCount, 1, 16);
    const float ultimateStompTelegraphSec = ClampRange(tran.gameplay.bossUltimateStompTelegraph, 0.10f, 12.0f) * telegraphMultiplier;
    const float ultimateStompRadiusScale = (tran.gameplay.bossUltimateStompRadiusScale < 0.5f) ? 0.5f : tran.gameplay.bossUltimateStompRadiusScale;
    const float ultimateFieldTelegraphSec = ClampRange(tran.gameplay.bossUltimateFieldTelegraph, 0.10f, 12.0f) * telegraphMultiplier;
    const float ultimateFieldSafeScale = (tran.gameplay.bossUltimateFieldSafeScale < 0.5f) ? 0.5f : tran.gameplay.bossUltimateFieldSafeScale;
    const DirectX::XMFLOAT4 dangerColor = { 1.0f, 0.15f, 0.10f, 1.0f };
    const DirectX::XMFLOAT4 safeColor = { 0.20f, 0.95f, 0.35f, 1.0f };
    const DirectX::XMFLOAT4 safeOutlineColor = { 1.0f, 1.0f, 1.0f, 1.0f };

    auto clampCenterToStage = [&](DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& size)
    {
        const float halfX = size.x * 0.5f;
        const float halfZ = size.z * 0.5f;
        center.x = ClampRange(center.x, -stageHalf + halfX, stageHalf - halfX);
        center.z = ClampRange(center.z, -stageHalf + halfZ, stageHalf - halfZ);
    };

    auto clampZoneSizeToStage = [&](DirectX::XMFLOAT3& size)
    {
        const float maxSpan = (stageSize > 0.10f) ? (stageSize - 0.05f) : 0.05f;
        size.x = ClampRange(size.x, 0.10f, maxSpan);
        size.z = ClampRange(size.z, 0.10f, maxSpan);
    };

    auto startTelegraph = [&](BossController::AttackKind kind, float telegraphDuration, float executeDuration)
    {
        m_boss.attackKind = kind;
        m_boss.attackState = BossController::AttackTelegraph;
        m_boss.attackStateTimer = 0.0f;
        m_boss.attackTelegraphDuration = telegraphDuration;
        m_boss.attackExecuteDuration = executeDuration;
        m_boss.attackResolved = false;
        m_boss.jumpedOut = false;
        m_boss.attackZones.clear();
        m_boss.attackLane.center = { 0.0f, 0.0f, 0.0f };
        m_boss.attackLane.size = { 0.0f, 0.0f, 0.0f };
        m_boss.pos = { 0.0f, 0.0f, 0.0f };
    };

    auto beginDashAttack = [&](BossController::AttackKind kind, bool wideLane)
    {
        startTelegraph(kind, wideLane ? dashWideTelegraphSec : dashNarrowTelegraphSec, dashSec);

        m_boss.attackPattern = ((std::rand() & 1) == 0)
            ? BossController::AttackPatternVertical
            : BossController::AttackPatternHorizontal;
        const bool verticalLane = (m_boss.attackPattern == BossController::AttackPatternVertical);

        float laneThickness = wideLane
            ? (stageSize * dashWideWidthRate * globalLaneScale)
            : (playerSpan * lanePlayerRatio);
        if (verticalLane)
        {
            laneThickness = ClampRange(laneThickness, playerWidth, stageSize - playerWidth);
        }
        else
        {
            laneThickness = ClampRange(laneThickness, playerDepth, stageSize - playerDepth);
        }

        m_boss.attackLane.pattern = m_boss.attackPattern;
        m_boss.attackLane.center = {
            verticalLane ? tran.player.pos.x : 0.0f,
            tran.player.pos.y + zoneHeight * 0.5f,
            verticalLane ? 0.0f : tran.player.pos.z
        };
        m_boss.attackLane.size = verticalLane
            ? DirectX::XMFLOAT3{ laneThickness, zoneHeight, stageSize }
            : DirectX::XMFLOAT3{ stageSize, zoneHeight, laneThickness };
        clampCenterToStage(m_boss.attackLane.center, m_boss.attackLane.size);

        PushAttackZone(m_boss.attackZones, m_boss.attackLane.center, m_boss.attackLane.size, dangerColor);

        const float outMargin = 0.65f;
        if (verticalLane)
        {
            m_boss.dashStartPos = { m_boss.attackLane.center.x, 0.0f, stageHalf + m_boss.size.z * outMargin };
            m_boss.dashEndPos = { m_boss.attackLane.center.x, 0.0f, -stageHalf - m_boss.size.z * outMargin };
        }
        else
        {
            m_boss.dashStartPos = { stageHalf + m_boss.size.x * outMargin, 0.0f, m_boss.attackLane.center.z };
            m_boss.dashEndPos = { -stageHalf - m_boss.size.x * outMargin, 0.0f, m_boss.attackLane.center.z };
        }
    };

    auto beginRandomRain = [&]()
    {
        startTelegraph(BossController::AttackKindRandomRain, randomRainTelegraphSec, 0.30f);
        const float rockSpan = ClampRange(playerSpan * randomRainRadiusScale * globalLaneScale, 0.10f, stageSize - 0.05f);
        const float halfRockSpan = rockSpan * 0.5f;
        for (int i = 0; i < randomRainCount; ++i)
        {
            DirectX::XMFLOAT3 center = {
                RandomRange(-stageHalf + halfRockSpan, stageHalf - halfRockSpan),
                tran.player.pos.y + zoneHeight * 0.5f,
                RandomRange(-stageHalf + halfRockSpan, stageHalf - halfRockSpan)
            };
            PushAttackZone(m_boss.attackZones, center, { rockSpan, zoneHeight, rockSpan }, dangerColor);
        }
    };

    auto beginSummon = [&]()
    {
        startTelegraph(BossController::AttackKindSummon, summonTelegraphSec, 0.20f);
        m_boss.attackRepeatsRemaining = RandomRangeInt(summonMin, summonMax);
        DirectX::XMFLOAT3 zoneSize = { playerSpan * 4.0f * globalLaneScale, zoneHeight, playerSpan * 4.0f * globalLaneScale };
        clampZoneSizeToStage(zoneSize);
        PushAttackZone(
            m_boss.attackZones,
            { 0.0f, tran.player.pos.y + zoneHeight * 0.5f, 0.0f },
            zoneSize,
            { 0.95f, 0.55f, 0.15f, 1.0f });
    };

    auto beginTrackingDrop = [&](bool startNewSequence)
    {
        if (startNewSequence)
        {
            m_boss.attackRepeatsRemaining = trackingDropCount;
        }

        startTelegraph(BossController::AttackKindTrackingDrop, trackingDropTelegraphSec, 0.28f);
        DirectX::XMFLOAT3 center = {
            tran.player.pos.x,
            tran.player.pos.y + zoneHeight * 0.5f,
            tran.player.pos.z
        };
        DirectX::XMFLOAT3 size = {
            playerSpan * trackingDropRadiusScale * globalLaneScale,
            zoneHeight,
            playerSpan * trackingDropRadiusScale * globalLaneScale
        };
        clampZoneSizeToStage(size);
        clampCenterToStage(center, size);
        PushAttackZone(m_boss.attackZones, center, size, dangerColor);
    };

    auto beginUltimateCross = [&]()
    {
        startTelegraph(BossController::AttackKindUltimateCross, ultimateCrossTelegraphSec, 0.22f);
        const float laneWidth = ClampRange(
            playerSpan * ultimateCrossLaneScale * globalLaneScale,
            0.10f,
            stageSize - 0.05f);
        const float offsets[2] = { -stageSize / 6.0f, stageSize / 6.0f };
        for (int i = 0; i < 2; ++i)
        {
            PushAttackZone(
                m_boss.attackZones,
                { offsets[i], tran.player.pos.y + zoneHeight * 0.5f, 0.0f },
                { laneWidth, zoneHeight, stageSize },
                dangerColor);
            PushAttackZone(
                m_boss.attackZones,
                { 0.0f, tran.player.pos.y + zoneHeight * 0.5f, offsets[i] },
                { stageSize, zoneHeight, laneWidth },
                dangerColor);
        }
    };

    auto beginUltimateStomp = [&](bool startNewSequence)
    {
        if (startNewSequence)
        {
            m_boss.attackRepeatsRemaining = ultimateStompCount;
        }

        startTelegraph(BossController::AttackKindUltimateStomp, ultimateStompTelegraphSec, 0.32f);
        DirectX::XMFLOAT3 center = {
            tran.player.pos.x,
            tran.player.pos.y + zoneHeight * 0.5f,
            tran.player.pos.z
        };
        DirectX::XMFLOAT3 size = {
            playerSpan * ultimateStompRadiusScale * globalLaneScale,
            zoneHeight,
            playerSpan * ultimateStompRadiusScale * globalLaneScale
        };
        clampZoneSizeToStage(size);
        clampCenterToStage(center, size);
        PushAttackZone(m_boss.attackZones, center, size, dangerColor);
        m_boss.dashStartPos = m_boss.pos;
        m_boss.dashEndPos = { center.x, 0.0f, center.z };
    };

    auto beginUltimateField = [&]()
    {
        startTelegraph(BossController::AttackKindUltimateField, ultimateFieldTelegraphSec, 0.35f);

        const float safeSpan = ClampRange(
            playerSpan * ultimateFieldSafeScale * globalLaneScale,
            0.10f,
            stageSize - 0.05f);
        const DirectX::XMFLOAT3 safeSize = { safeSpan, zoneHeight, safeSpan };
        const DirectX::XMFLOAT3 safeCandidates[3] =
        {
            { 0.0f, tran.player.pos.y + zoneHeight * 0.5f, 0.0f },
            { -stageHalf * 0.45f, tran.player.pos.y + zoneHeight * 0.5f, stageHalf * 0.45f },
            { stageHalf * 0.45f, tran.player.pos.y + zoneHeight * 0.5f, -stageHalf * 0.45f }
        };
        DirectX::XMFLOAT3 safeCenter = safeCandidates[std::rand() % 3];
        clampCenterToStage(safeCenter, safeSize);

        PushAttackZone(
            m_boss.attackZones,
            safeCenter,
            { safeSize.x * 1.18f, safeSize.y, safeSize.z * 1.18f },
            safeOutlineColor,
            0.0f,
            true);
        PushAttackZone(m_boss.attackZones, safeCenter, safeSize, safeColor, 0.0f, true);

        const int gridCount = 7;
        const float cellSize = stageSize / static_cast<float>(gridCount);
        std::vector<DirectX::XMFLOAT3> fillCenters;
        fillCenters.reserve(gridCount * gridCount);
        for (int z = 0; z < gridCount; ++z)
        {
            for (int x = 0; x < gridCount; ++x)
            {
                const DirectX::XMFLOAT3 center = {
                    -stageHalf + cellSize * (0.5f + static_cast<float>(x)),
                    tran.player.pos.y + zoneHeight * 0.5f,
                    -stageHalf + cellSize * (0.5f + static_cast<float>(z))
                };
                const bool overlapsSafe =
                    (std::fabs(center.x - safeCenter.x) < (cellSize * 0.5f + safeSize.x * 0.5f)) &&
                    (std::fabs(center.z - safeCenter.z) < (cellSize * 0.5f + safeSize.z * 0.5f));
                if (!overlapsSafe)
                {
                    fillCenters.push_back(center);
                }
            }
        }

        const int fillCount = static_cast<int>(fillCenters.size());
        for (int i = 0; i < fillCount; ++i)
        {
            float revealStart = 0.0f;
            if (fillCount > 1)
            {
                revealStart = (static_cast<float>(i) / static_cast<float>(fillCount - 1)) * (6.0f / 7.0f);
            }
            PushAttackZone(
                m_boss.attackZones,
                fillCenters[i],
                { cellSize * 1.02f, zoneHeight, cellSize * 1.02f },
                dangerColor,
                revealStart,
                false);
        }
    };

    auto beginNextTopLevelAttack = [&]()
    {
        ++m_boss.attackCycleCount;
        if ((m_boss.attackCycleCount % 5) == 0)
        {
            beginUltimateCross();
            return;
        }

        switch (std::rand() % 5)
        {
        case 0: beginDashAttack(BossController::AttackKindDashNarrow, false); break;
        case 1: beginDashAttack(BossController::AttackKindDashWide, true); break;
        case 2: beginRandomRain(); break;
        case 3: beginSummon(); break;
        default: beginTrackingDrop(true); break;
        }
    };

    auto applyAttackDamage = [&]() -> bool
    {
        Collision::Box playerBox = MakeAabb(
            {
                tran.player.pos.x,
                tran.player.pos.y + tran.player.size.y * 0.5f,
                tran.player.pos.z
            },
            tran.player.size);

        bool safeFromField = false;
        for (const auto& zone : m_boss.attackZones)
        {
            if (!zone.safeZone)
            {
                continue;
            }

            Collision::Box safeBox = MakeAabb(
                { zone.center.x, playerBox.center.y, zone.center.z },
                { zone.size.x, zoneHeight, zone.size.z });
            if (HitAabb(playerBox, safeBox))
            {
                safeFromField = true;
                break;
            }
        }

        switch (m_boss.attackKind)
        {
        case BossController::AttackKindSummon:
            for (int i = 0; i < m_boss.attackRepeatsRemaining; ++i)
            {
                SpawnEnemyByIndex(static_cast<int>(m_enemies.size()), stageSize);
            }
            break;

        case BossController::AttackKindUltimateField:
            if (!safeFromField && applyPlayerDamage && applyPlayerDamage(bossDamage))
            {
                return true;
            }
            break;

        case BossController::AttackKindRandomRain:
        case BossController::AttackKindTrackingDrop:
            for (const auto& zone : m_boss.attackZones)
            {
                if (zone.safeZone) continue;
                const Collision::Box hitBox = MakeAabb(
                    { zone.center.x, playerBox.center.y, zone.center.z },
                    { zone.size.x, zoneHeight, zone.size.z });
                if (HitAabb(playerBox, hitBox))
                {
                    if (applyPlayerDamage && applyPlayerDamage(bossDamage))
                    {
                        return true;
                    }
                    break;
                }
            }
            for (const auto& zone : m_boss.attackZones)
            {
                if (!zone.safeZone)
                {
                    SpawnRockVisual(m_boss.fallingRocks, zone.center, MaxFloat(zone.size.x, zone.size.z));
                }
            }
            break;

        case BossController::AttackKindUltimateStomp:
            for (const auto& zone : m_boss.attackZones)
            {
                if (zone.safeZone) continue;
                const Collision::Box hitBox = MakeAabb(
                    { zone.center.x, playerBox.center.y, zone.center.z },
                    { zone.size.x, zoneHeight, zone.size.z });
                if (HitAabb(playerBox, hitBox))
                {
                    if (applyPlayerDamage && applyPlayerDamage(bossDamage))
                    {
                        return true;
                    }
                    break;
                }
            }
            break;

        default:
            for (const auto& zone : m_boss.attackZones)
            {
                if (zone.safeZone) continue;
                const Collision::Box hitBox = MakeAabb(
                    { zone.center.x, playerBox.center.y, zone.center.z },
                    { zone.size.x, zoneHeight, zone.size.z });
                if (HitAabb(playerBox, hitBox))
                {
                    if (applyPlayerDamage && applyPlayerDamage(bossDamage))
                    {
                        return true;
                    }
                    break;
                }
            }
            break;
        }

        return false;
    };

    auto finishCurrentAttack = [&]()
    {
        m_boss.attackZones.clear();
        m_boss.attackLane.center = { 0.0f, 0.0f, 0.0f };
        m_boss.attackLane.size = { 0.0f, 0.0f, 0.0f };
        m_boss.attackState = BossController::AttackIdle;
        m_boss.attackStateTimer = 0.0f;
        m_boss.attackTelegraphDuration = 0.0f;
        m_boss.attackExecuteDuration = 0.0f;
        m_boss.attackResolved = false;
        m_boss.jumpedOut = false;
        m_boss.pos = { 0.0f, 0.0f, 0.0f };
        m_boss.attackCooldownTimer = cooldownSec;
    };

    auto advanceBossSequence = [&]()
    {
        const DirectX::XMFLOAT3 previousPos = m_boss.pos;
        m_boss.attackZones.clear();
        m_boss.attackLane.center = { 0.0f, 0.0f, 0.0f };
        m_boss.attackLane.size = { 0.0f, 0.0f, 0.0f };
        m_boss.attackStateTimer = 0.0f;
        m_boss.attackTelegraphDuration = 0.0f;
        m_boss.attackExecuteDuration = 0.0f;
        m_boss.attackResolved = false;
        m_boss.jumpedOut = false;
        m_boss.pos = { 0.0f, 0.0f, 0.0f };

        switch (m_boss.attackKind)
        {
        case BossController::AttackKindTrackingDrop:
            --m_boss.attackRepeatsRemaining;
            if (m_boss.attackRepeatsRemaining > 0)
            {
                beginTrackingDrop(false);
                return;
            }
            break;

        case BossController::AttackKindUltimateCross:
            beginUltimateStomp(true);
            m_boss.dashStartPos = previousPos;
            m_boss.pos = previousPos;
            return;

        case BossController::AttackKindUltimateStomp:
            --m_boss.attackRepeatsRemaining;
            if (m_boss.attackRepeatsRemaining > 0)
            {
                beginUltimateStomp(false);
                m_boss.dashStartPos = previousPos;
                m_boss.pos = previousPos;
                return;
            }
            beginUltimateField();
            return;

        default:
            break;
        }

        finishCurrentAttack();
    };

    if (m_boss.hp > 0)
    {
        if (m_boss.attackState == BossController::AttackIdle)
        {
            m_boss.pos = { 0.0f, 0.0f, 0.0f };
            if (m_boss.attackCooldownTimer > 0.0f)
            {
                m_boss.attackCooldownTimer -= kBossFixedDt;
                if (m_boss.attackCooldownTimer < 0.0f) m_boss.attackCooldownTimer = 0.0f;
            }
            else
            {
                beginNextTopLevelAttack();
            }
        }
        else if (m_boss.attackState == BossController::AttackTelegraph)
        {
            m_boss.attackStateTimer += kBossFixedDt;

            const bool isDashAttack =
                (m_boss.attackKind == BossController::AttackKindDashNarrow) ||
                (m_boss.attackKind == BossController::AttackKindDashWide);
            const bool isStompAttack = (m_boss.attackKind == BossController::AttackKindUltimateStomp);
            const float effectiveJumpOutSec = (jumpOutSec < m_boss.attackTelegraphDuration)
                ? jumpOutSec
                : m_boss.attackTelegraphDuration;
            const float stompJumpStartLimit = (m_boss.attackTelegraphDuration > kBossFixedDt)
                ? (m_boss.attackTelegraphDuration - kBossFixedDt)
                : 0.0f;
            const float stompJumpStartSec = ClampRange(
                MaxFloat(effectiveJumpOutSec, m_boss.attackTelegraphDuration * 0.60f),
                0.0f,
                stompJumpStartLimit);

            if (isDashAttack)
            {
                if (!m_boss.jumpedOut && m_boss.attackStateTimer >= effectiveJumpOutSec)
                {
                    m_boss.pos = m_boss.dashStartPos;
                    m_boss.jumpedOut = true;
                }
            }
            else if (isStompAttack && !m_boss.attackZones.empty())
            {
                if (m_boss.attackStateTimer < stompJumpStartSec)
                {
                    m_boss.pos = m_boss.dashStartPos;
                }
                else
                {
                    const float jumpDuration = (m_boss.attackTelegraphDuration > stompJumpStartSec)
                        ? (m_boss.attackTelegraphDuration - stompJumpStartSec)
                        : kBossFixedDt;
                    const float jumpRate = Clamp01((m_boss.attackStateTimer - stompJumpStartSec) / jumpDuration);
                    const float heightRate = jumpRate * jumpRate * (3.0f - 2.0f * jumpRate);
                    DirectX::XMFLOAT3 jumpPos = LerpFloat3(m_boss.dashStartPos, m_boss.dashEndPos, jumpRate);
                    jumpPos.y = m_boss.size.y * 2.4f * heightRate;
                    m_boss.pos = jumpPos;
                }
            }

            if (m_boss.attackStateTimer >= m_boss.attackTelegraphDuration)
            {
                m_boss.attackState = BossController::AttackDash;
                m_boss.attackStateTimer = 0.0f;
                m_boss.attackResolved = isStompAttack ? false : true;

                if (isDashAttack)
                {
                    m_boss.pos = m_boss.dashStartPos;
                }
                else if (isStompAttack && !m_boss.attackZones.empty())
                {
                    m_boss.pos = {
                        m_boss.dashEndPos.x,
                        m_boss.size.y * 2.4f,
                        m_boss.dashEndPos.z
                    };
                }
                else
                {
                    m_boss.pos = { 0.0f, 0.0f, 0.0f };
                }

                if (!isStompAttack && applyAttackDamage())
                {
                    return true;
                }
            }
        }
        else
        {
            m_boss.attackStateTimer += kBossFixedDt;

            if (m_boss.attackKind == BossController::AttackKindDashNarrow ||
                m_boss.attackKind == BossController::AttackKindDashWide)
            {
                const float duration = (m_boss.attackExecuteDuration > 0.01f) ? m_boss.attackExecuteDuration : 0.01f;
                const float dashRate = Clamp01(m_boss.attackStateTimer / duration);
                m_boss.pos = LerpFloat3(m_boss.dashStartPos, m_boss.dashEndPos, dashRate);
            }
            else if (m_boss.attackKind == BossController::AttackKindUltimateStomp &&
                     !m_boss.attackZones.empty())
            {
                const float duration = (m_boss.attackExecuteDuration > 0.01f) ? m_boss.attackExecuteDuration : 0.01f;
                const float fallRate = Clamp01(m_boss.attackStateTimer / duration);
                const DirectX::XMFLOAT3 fallStartPos = {
                    m_boss.dashEndPos.x,
                    m_boss.size.y * 2.4f,
                    m_boss.dashEndPos.z
                };
                m_boss.pos = LerpFloat3(fallStartPos, m_boss.dashEndPos, fallRate);
            }
            else
            {
                m_boss.pos = { 0.0f, 0.0f, 0.0f };
            }

            if (m_boss.attackStateTimer >= m_boss.attackExecuteDuration)
            {
                if (m_boss.attackKind == BossController::AttackKindUltimateStomp && !m_boss.attackResolved)
                {
                    const float stompImpactDuration = 0.18f;
                    const float stompShakeDuration = ClampRange(
                        MaxFloat(tran.gameplay.screenShakeDuration, 0.14f),
                        0.08f,
                        0.40f);
                    const float stompShakeAmplitude = ClampRange(
                        MaxFloat(tran.gameplay.screenShakeAmplitude, 0.16f) * 1.35f,
                        0.10f,
                        0.80f);
                    m_boss.pos = m_boss.dashEndPos;
                    m_boss.attackResolved = true;
                    m_bossStompImpactTimer = stompImpactDuration;
                    m_screenShakeDuration = stompShakeDuration;
                    if (m_screenShakeTimer < stompShakeDuration)
                    {
                        m_screenShakeTimer = stompShakeDuration;
                    }
                    if (m_screenShakeAmplitude < stompShakeAmplitude)
                    {
                        m_screenShakeAmplitude = stompShakeAmplitude;
                    }
                    m_screenShakePhase += 1.0f;
                    if (applyAttackDamage())
                    {
                        return true;
                    }
                }
                advanceBossSequence();
            }
        }
    }
    else
    {
        m_boss.pos = { 0.0f, 0.0f, 0.0f };
        m_boss.attackZones.clear();
        m_boss.attackState = BossController::AttackIdle;
        m_boss.attackStateTimer = 0.0f;
        m_boss.attackTelegraphDuration = 0.0f;
        m_boss.attackExecuteDuration = 0.0f;
        m_boss.attackCooldownTimer = 0.0f;
        m_boss.attackResolved = false;
        m_boss.jumpedOut = false;
    }

    if (m_boss.hp > 0 && m_attackActive && m_boss.lastHitSwingId != m_attackSwingId)
    {
        Collision::Box attackBox{};
        attackBox.center = m_attackCenter;
        attackBox.size = m_attackSize;
        Collision::Box bossBox = MakeAabb({
            m_boss.pos.x,
            m_boss.pos.y + m_boss.size.y * 0.5f,
            m_boss.pos.z
        }, m_boss.size);
        if (HitAabb(attackBox, bossBox))
        {
            m_boss.hp -= playerAttackDamage;
            if (m_boss.hp < 0) m_boss.hp = 0;
            m_boss.lastHitSwingId = m_attackSwingId;
            ++m_attackHitCountThisSwing;
            if (m_pAttackSe) PlaySound(m_pAttackSe);
            if (m_boss.hp <= 0)
            {
                m_boss.attackZones.clear();
                m_boss.fallingRocks.clear();
                tran.gameplayDebug.runTimerRunning = 0;
                tran.gameplayDebug.runRecordedSec = tran.gameplayDebug.runElapsedSec;
                tran.gameplayDebug.bossBattleActive = 0;
                tran.gameplayDebug.showBossResultTimer = 1;
                tran.gameplayDebug.upgradeSelectionPending = 0;
                tran.gameplayDebug.upgradeRerollRemain = 0;
                tran.roguelike.selectionPending = 0;
                tran.roguelike.rerollRemain = 0;
                if (m_pClearSe) PlaySound(m_pClearSe);
                SceneManager::ChangeResult(SceneManager::ResultType::Win);
                SceneManager::ChangeScene(SceneManager::SCENE_RESULT);
                return true;
            }
        }
    }

    return false;
}

void SceneGame::DrawBossTelegraphMarker() const
{
    if (!m_pBossAttackRangeMarker ||
        !m_isBossBattleDebug ||
        m_boss.hp <= 0 ||
        m_boss.attackState != BossController::AttackTelegraph ||
        m_boss.attackZones.empty())
    {
        return;
    }

    const float telegraphSec = (m_boss.attackTelegraphDuration > 0.01f)
        ? m_boss.attackTelegraphDuration
        : 0.01f;
    const float telegraphRate = Clamp01(m_boss.attackStateTimer / telegraphSec);
    for (const auto& zone : m_boss.attackZones)
    {
        if (telegraphRate + 0.0001f < zone.revealStart)
        {
            continue;
        }

        float localRate = 1.0f;
        if (zone.revealStart < 0.999f)
        {
            localRate = Clamp01((telegraphRate - zone.revealStart) / (1.0f - zone.revealStart));
        }
        const float pulse = 0.55f + 0.45f * static_cast<float>(std::sin((telegraphRate + localRate) * DirectX::XM_PI * 6.0f));

        if (zone.safeZone)
        {
            const DirectX::XMFLOAT3 outlineSize = { zone.size.x * 1.06f, zone.size.y, zone.size.z * 1.06f };
            DrawAttackMarkerTintLocal(
                m_pBossAttackRangeMarker,
                zone.center,
                outlineSize,
                { 1.0f, 1.0f, 1.0f, 0.10f + 0.08f * pulse });
        }

        DirectX::XMFLOAT4 color = zone.color;
        const float baseAlpha = zone.safeZone ? 0.08f : 0.14f;
        const float pulseAlpha = zone.safeZone ? 0.20f : 0.34f;
        color.w = baseAlpha + pulseAlpha * pulse;
        DrawAttackMarkerTintLocal(
            m_pBossAttackRangeMarker,
            zone.center,
            zone.size,
            color);
    }
}

void SceneGame::DrawBossFallingObjects() const
{
    if (!m_isBossBattleDebug || !m_boss.rockTexture)
    {
        return;
    }

    if (m_boss.hp > 0 &&
        m_boss.attackState == BossController::AttackTelegraph &&
        m_boss.attackKind == BossController::AttackKindUltimateField)
    {
        const float telegraphSec = (m_boss.attackTelegraphDuration > 0.01f)
            ? m_boss.attackTelegraphDuration
            : 0.01f;
        const float telegraphRate = Clamp01(m_boss.attackStateTimer / telegraphSec);
        const float finalDropStartRate = 6.0f / 7.0f;
        if (telegraphRate >= finalDropStartRate)
        {
            const float dropRate = Clamp01((telegraphRate - finalDropStartRate) / (1.0f - finalDropStartRate));
            for (const auto& zone : m_boss.attackZones)
            {
                if (zone.safeZone)
                {
                    continue;
                }

                const float rockSpan = MaxFloat(zone.size.x, zone.size.z) * 0.82f;
                DirectX::XMFLOAT3 drawPos = zone.center;
                drawPos.y = rockSpan * (0.35f + 2.20f * (1.0f - dropRate));

                DrawBillboardSpriteLocal(
                    m_boss.rockTexture,
                    m_pCamera,
                    drawPos,
                    { rockSpan, rockSpan, rockSpan },
                    { 1.0f, 1.0f, 1.0f, 0.35f + 0.55f * dropRate });
            }
        }
    }

    for (const auto& rock : m_boss.fallingRocks)
    {
        if (rock.timer <= 0.0f || rock.duration <= 0.0f)
        {
            continue;
        }

        const float t = Clamp01(rock.timer / rock.duration);
        DirectX::XMFLOAT3 drawPos = rock.pos;
        drawPos.y = rock.size.y * (0.10f + 2.80f * t);

        DrawBillboardSpriteLocal(
            m_boss.rockTexture,
            m_pCamera,
            drawPos,
            rock.size,
            { 1.0f, 1.0f, 1.0f, 0.45f + 0.55f * (1.0f - t) });
    }
}

void SceneGame::AddBossDrawEntry(std::vector<DrawEntry>& drawEntries, const DirectX::XMFLOAT3& cam) const
{
    if (!m_isBossBattleDebug || m_boss.hp <= 0 || !m_boss.texture)
    {
        return;
    }

    const DirectX::XMFLOAT3 bossCenter = {
        m_boss.pos.x,
        m_boss.pos.y + m_boss.size.y * 0.5f,
        m_boss.pos.z
    };
    const float dx = bossCenter.x - cam.x;
    const float dy = bossCenter.y - cam.y;
    const float dz = bossCenter.z - cam.z;
    drawEntries.push_back({ dx * dx + dy * dy + dz * dz, false, true, bossCenter, m_boss.size, nullptr });
}

void SceneGame::DrawBossEntry(const DrawEntry& entry) const
{
    if (!entry.isBoss || !m_boss.texture)
    {
        return;
    }

    const DirectX::XMFLOAT3 bossBottom = {
        entry.pos.x,
        entry.pos.y - entry.size.y * 0.5f,
        entry.pos.z
    };
    DrawBillboardSpriteLocal(m_boss.texture, m_pCamera, bossBottom, entry.size, m_boss.color);
}

void SceneGame::DrawBossHpUi() const
{
    if (!m_isBossBattleDebug || m_boss.maxHp <= 0)
    {
        return;
    }

    auto& tran = Transfer::GetInstance();
    const float hpRate = Clamp01(static_cast<float>(m_boss.hp) / static_cast<float>(m_boss.maxHp));
    DrawBossHpOverlayLocal(hpRate, tran.gameplay.bossHpBarWidthRate, tran.gameplay.bossHpBarHeightRate);
}


