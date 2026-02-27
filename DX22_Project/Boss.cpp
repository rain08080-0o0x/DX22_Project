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
#include <cmath>
#include <cstdlib>

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
    attackPattern = AttackPatternVertical;
    attackState = AttackIdle;
    attackStateTimer = 0.0f;
    attackCooldownTimer = 0.0f;
    patternDecisionTimer = 0.0f;
    jumpedOut = false;
    attackLane.center = { 0.0f, 0.0f, 0.0f };
    attackLane.size = { 0.0f, 0.0f, 0.0f };
    attackLane.pattern = attackPattern;
}

void BossController::LoadTexture(const char* path)
{
    ReleaseTexture();
    texture = new Texture();
    if (!texture || FAILED(texture->Create(path)))
    {
        MessageBox(NULL, "Texture load failed.\nBoss texture", "Error", MB_OK);
    }
}

void BossController::ReleaseTexture()
{
    if (texture)
    {
        delete texture;
        texture = nullptr;
    }
}

void SceneGame::InitializeBossForScene()
{
    auto& tran = Transfer::GetInstance();
    m_boss.ResetForScene(tran.player.size, tran.gameplay.bossSizeAreaScale, tran.gameplay.bossMaxHp);
}

void SceneGame::LoadBossResources()
{
    m_boss.LoadTexture("Assets/Texture/Chracter/genbaneko.png");
}

void SceneGame::ReleaseBossResources()
{
    m_boss.ReleaseTexture();
}

bool SceneGame::UpdateBossDebugSetup(float stageSize)
{
    if (!m_isBossBattleDebug)
    {
        return false;
    }

    auto& tran = Transfer::GetInstance();
    if (!m_enemies.empty())
    {
        EnsureEnemyCount(0, stageSize);
    }
    if (!m_enemyProjectiles.empty())
    {
        m_enemyProjectiles.clear();
    }
    m_requestedEnemyCount = 0;
    tran.gameplayDebug.bossBattleActive = 1;
    tran.gameplayDebug.bossHp = static_cast<float>(m_boss.hp);
    tran.gameplayDebug.bossMaxHp = static_cast<float>(m_boss.maxHp);
    tran.gameplay.bossHpBarWidthRate = ClampRange(tran.gameplay.bossHpBarWidthRate, 0.20f, 0.90f);
    tran.gameplay.bossHpBarHeightRate = ClampRange(tran.gameplay.bossHpBarHeightRate, 0.01f, 0.20f);
    tran.gameplay.bossSizeAreaScale = ClampRange(tran.gameplay.bossSizeAreaScale, 4.0f, 12.0f);
    tran.gameplay.bossAttackTelegraph = ClampRange(tran.gameplay.bossAttackTelegraph, 0.10f, 4.0f);
    tran.gameplay.bossAttackJumpOutTime = ClampRange(
        tran.gameplay.bossAttackJumpOutTime,
        0.0f,
        tran.gameplay.bossAttackTelegraph);
    tran.gameplay.bossAttackDashDuration = ClampRange(tran.gameplay.bossAttackDashDuration, 0.05f, 2.0f);
    tran.gameplay.bossAttackCooldown = ClampRange(tran.gameplay.bossAttackCooldown, 0.0f, 6.0f);
    tran.gameplay.bossAttackLanePlayerScale = ClampRange(tran.gameplay.bossAttackLanePlayerScale, 0.5f, 8.0f);
    if (tran.gameplay.bossAttackDamage < 0.0f) tran.gameplay.bossAttackDamage = 0.0f;
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
    const float stageHalf = stageSize * 0.5f;
    const float laneScale = 3.0f; // Requested spec: lane width = about 3 player widths.
    const float telegraphSec = tran.gameplay.bossAttackTelegraph;
    const float jumpOutSec = tran.gameplay.bossAttackJumpOutTime;
    const float dashSec = tran.gameplay.bossAttackDashDuration;
    const float cooldownSec = tran.gameplay.bossAttackCooldown;
    const float bossDamage = tran.gameplay.bossAttackDamage;
    const float laneHeight = (m_boss.size.y > tran.player.size.y) ? m_boss.size.y : tran.player.size.y;
    const float nearDistance = stageSize * 0.22f;
    const float farDistance = stageSize * 0.40f;
    const float patternSwitchSec = 0.75f;

    auto decideNextPattern = [&]() -> BossController::AttackPattern
    {
        const float dx = tran.player.pos.x - m_boss.pos.x;
        const float dz = tran.player.pos.z - m_boss.pos.z;
        const float distSq = dx * dx + dz * dz;
        const float nearSq = nearDistance * nearDistance;
        const float farSq = farDistance * farDistance;

        if (distSq <= nearSq)
        {
            return BossController::AttackPatternHorizontal;
        }
        if (distSq >= farSq)
        {
            return BossController::AttackPatternVertical;
        }
        if (m_boss.patternDecisionTimer >= patternSwitchSec)
        {
            return (m_boss.attackPattern == BossController::AttackPatternVertical)
                ? BossController::AttackPatternHorizontal
                : BossController::AttackPatternVertical;
        }
        return ((std::rand() & 1) == 0)
            ? BossController::AttackPatternVertical
            : BossController::AttackPatternHorizontal;
    };

    auto beginBossAttack = [&]()
    {
        m_boss.attackPattern = decideNextPattern();
        m_boss.attackLane.pattern = m_boss.attackPattern;
        const bool verticalLane = (m_boss.attackPattern == BossController::AttackPatternVertical);

        const float laneThickness = ((verticalLane ? tran.player.size.x : tran.player.size.z) * laneScale);
        const float safeThickness = (laneThickness < 0.1f) ? 0.1f : laneThickness;
        m_boss.attackLane.center = {
            verticalLane ? tran.player.pos.x : 0.0f,
            tran.player.pos.y + laneHeight * 0.5f,
            verticalLane ? 0.0f : tran.player.pos.z
        };
        m_boss.attackLane.size = verticalLane
            ? DirectX::XMFLOAT3{ safeThickness, laneHeight, stageSize }
            : DirectX::XMFLOAT3{ stageSize, laneHeight, safeThickness };

        if (verticalLane)
        {
            const float minX = -stageHalf + safeThickness * 0.5f;
            const float maxX = stageHalf - safeThickness * 0.5f;
            m_boss.attackLane.center.x = ClampRange(m_boss.attackLane.center.x, minX, maxX);
        }
        else
        {
            const float minZ = -stageHalf + safeThickness * 0.5f;
            const float maxZ = stageHalf - safeThickness * 0.5f;
            m_boss.attackLane.center.z = ClampRange(m_boss.attackLane.center.z, minZ, maxZ);
        }

        const float outMargin = 0.60f;
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

        m_boss.attackState = BossController::AttackTelegraph;
        m_boss.attackStateTimer = 0.0f;
        m_boss.patternDecisionTimer = 0.0f;
        m_boss.jumpedOut = false;
        m_boss.pos = { 0.0f, 0.0f, 0.0f };
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
                m_boss.patternDecisionTimer += kBossFixedDt;
            }
            else
            {
                beginBossAttack();
            }
        }
        else if (m_boss.attackState == BossController::AttackTelegraph)
        {
            m_boss.attackStateTimer += kBossFixedDt;
            if (!m_boss.jumpedOut && m_boss.attackStateTimer >= jumpOutSec)
            {
                m_boss.pos = m_boss.dashStartPos;
                m_boss.jumpedOut = true;
            }
            if (m_boss.attackStateTimer >= telegraphSec)
            {
                m_boss.attackState = BossController::AttackDash;
                m_boss.attackStateTimer = 0.0f;
                m_boss.pos = m_boss.dashStartPos;
            }
        }
        else
        {
            m_boss.attackStateTimer += kBossFixedDt;
            const float dashRate = Clamp01(m_boss.attackStateTimer / dashSec);
            m_boss.pos = LerpFloat3(m_boss.dashStartPos, m_boss.dashEndPos, dashRate);

            Collision::Box playerBox = MakeAabb({
                tran.player.pos.x,
                tran.player.pos.y + tran.player.size.y * 0.5f,
                tran.player.pos.z
            }, tran.player.size);
            Collision::Box laneBox = MakeAabb(
            {
                m_boss.attackLane.center.x,
                playerBox.center.y,
                m_boss.attackLane.center.z
            },
            {
                m_boss.attackLane.size.x,
                laneHeight,
                m_boss.attackLane.size.z
            });
            if (HitAabb(playerBox, laneBox))
            {
                if (applyPlayerDamage && applyPlayerDamage(bossDamage))
                {
                    return true;
                }
            }

            if (m_boss.attackStateTimer >= dashSec)
            {
                m_boss.attackState = BossController::AttackIdle;
                m_boss.attackStateTimer = 0.0f;
                m_boss.attackCooldownTimer = cooldownSec;
                m_boss.pos = { 0.0f, 0.0f, 0.0f };
                m_boss.jumpedOut = false;
            }
        }
    }
    else
    {
        m_boss.pos = { 0.0f, 0.0f, 0.0f };
        m_boss.attackState = BossController::AttackIdle;
        m_boss.attackStateTimer = 0.0f;
        m_boss.attackCooldownTimer = 0.0f;
        m_boss.patternDecisionTimer = 0.0f;
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
        m_boss.attackLane.size.x <= 0.0f ||
        m_boss.attackLane.size.z <= 0.0f)
    {
        return;
    }

    auto& tran = Transfer::GetInstance();
    const float telegraphSec = (tran.gameplay.bossAttackTelegraph > 0.01f) ? tran.gameplay.bossAttackTelegraph : 0.01f;
    const float telegraphRate = Clamp01(m_boss.attackStateTimer / telegraphSec);
    const float pulse = 0.55f + 0.45f * static_cast<float>(std::sin(telegraphRate * DirectX::XM_PI * 6.0f));
    const float alpha = 0.18f + 0.30f * pulse;
    DrawAttackMarkerTintLocal(
        m_pBossAttackRangeMarker,
        m_boss.attackLane.center,
        m_boss.attackLane.size,
        { 1.0f, 0.15f, 0.10f, alpha });
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


