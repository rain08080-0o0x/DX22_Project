#ifndef __BOSS_H__
#define __BOSS_H__

#include <DirectXMath.h>

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
    void ReleaseTexture();

    Texture* texture = nullptr;
    DirectX::XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 size = { 2.0f, 2.0f, 2.0f };
    DirectX::XMFLOAT4 color = { 0.07f, 0.07f, 0.07f, 1.0f };
    int hp = 1;
    int maxHp = 1;
    int lastHitSwingId = -1;
    AttackPattern attackPattern = AttackPatternVertical;
    AttackLane attackLane;
    AttackState attackState = AttackIdle;
    float attackStateTimer = 0.0f;
    float attackCooldownTimer = 0.0f;
    float patternDecisionTimer = 0.0f;
    DirectX::XMFLOAT3 dashStartPos = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 dashEndPos = { 0.0f, 0.0f, 0.0f };
    bool jumpedOut = false;
};

#endif // __BOSS_H__

