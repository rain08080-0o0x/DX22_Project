#ifndef __BOSS_H__
#define __BOSS_H__

#include <DirectXMath.h>
#include <vector>

class Texture;

class BossController
{
public:
    enum AttackPattern
    {
        AttackPatternVertical = 0,
        AttackPatternHorizontal
    };

    enum AttackState
    {
        AttackIdle = 0,
        AttackTelegraph,
        AttackDash
    };

    enum AttackKind
    {
        AttackKindDashNarrow = 0,
        AttackKindDashWide,
        AttackKindRandomRain,
        AttackKindSummon,
        AttackKindTrackingDrop,
        AttackKindUltimateCross,
        AttackKindUltimateStomp,
        AttackKindUltimateField
    };

    struct AttackZone
    {
        DirectX::XMFLOAT3 center = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 size = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT4 color = { 1.0f, 0.15f, 0.10f, 1.0f };
        float revealStart = 0.0f;
        bool safeZone = false;
    };

    struct FallingRock
    {
        DirectX::XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 size = { 1.0f, 1.0f, 1.0f };
        float timer = 0.0f;
        float duration = 0.0f;
    };

    struct AttackLane
    {
        DirectX::XMFLOAT3 center = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 size = { 0.0f, 0.0f, 0.0f };
        AttackPattern pattern = AttackPatternVertical;
    };

    BossController() = default;
    ~BossController() = default;

    void ResetForScene(const DirectX::XMFLOAT3& playerSize, float bossSizeAreaScale, int bossMaxHp);
    void LoadTexture(const char* path);
    void LoadRockTexture(const char* path);
    void ReleaseTexture();
    void ReleaseRockTexture();

    Texture* texture = nullptr;
    Texture* rockTexture = nullptr;
    DirectX::XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 size = { 2.0f, 2.0f, 2.0f };
    DirectX::XMFLOAT4 color = { 0.07f, 0.07f, 0.07f, 1.0f };
    int hp = 1;
    int maxHp = 1;
    int lastHitSwingId = -1;
    AttackKind attackKind = AttackKindDashNarrow;
    AttackPattern attackPattern = AttackPatternVertical;
    AttackLane attackLane;
    std::vector<AttackZone> attackZones;
    std::vector<FallingRock> fallingRocks;
    AttackState attackState = AttackIdle;
    float attackStateTimer = 0.0f;
    float attackTelegraphDuration = 0.0f;
    float attackExecuteDuration = 0.0f;
    float attackCooldownTimer = 0.0f;
    int attackRepeatsRemaining = 0;
    int attackCycleCount = 0;
    DirectX::XMFLOAT3 dashStartPos = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 dashEndPos = { 0.0f, 0.0f, 0.0f };
    bool attackResolved = false;
    bool jumpedOut = false;
    bool requiresArenaReset = true;
};

#endif // __BOSS_H__
