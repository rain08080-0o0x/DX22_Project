#include "Enemy.h"
#include <cmath>
#include <cstdlib>

namespace
{
    const char* kEnemyTexture = "Assets/Texture/Chracter/genbaneko.png";
    const int kDefaultEnemyHp = 3;
    const float kDefaultStageSize = 5.0f;
    const float kDefaultMoveSpeed = 1.2f;
    const float kDefaultMoveSpeedScale = 1.0f;
    const float kMoveDt = 1.0f / 60.0f;
    const float kChaseStartRatio = 0.45f;
    const float kChaseEndRatio = 0.60f;
    const float kStopDistanceMin = 0.15f;
    const float kStopDistanceRange = 0.40f;
    const float kWanderSpeedScale = 0.50f;
    const float kWanderReachEps = 0.15f;
    const float kWanderTimerMin = 0.40f;
    const float kWanderTimerMax = 1.20f;

    float ClampFloat(float v, float lo, float hi)
    {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    float Rand01()
    {
        return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }

    float RandRange(float minV, float maxV)
    {
        return minV + (maxV - minV) * Rand01();
    }
}

Enemy::Enemy()
    : m_pTexture(nullptr)
    , m_size(0.5f, 1.0f, 0.5f)
    , m_color(1.0f, 0.25f, 0.25f, 1.0f)
    , m_moveSpeed(kDefaultMoveSpeed)
    , m_moveSpeedBase(kDefaultMoveSpeed)
    , m_moveSpeedScale(kDefaultMoveSpeedScale)
    , m_stageSize(kDefaultStageSize)
    , m_moveDirX(1.0f)
    , m_hp(kDefaultEnemyHp)
    , m_maxHp(kDefaultEnemyHp)
    , m_type(Type::Speed)
    , m_attackRangeScale(1.0f)
    , m_attackWindupScale(1.0f)
    , m_attackCooldownScale(1.0f)
    , m_attackDamageScale(1.0f)
    , m_targetPos({2,0,0})
    , m_wanderTarget(0.0f, 0.0f, 0.0f)
    , m_wanderTimer(0.0f)
    , m_state(MoveState::Wander)
{
    m_pos = { 0.0f, 0.0f, 0.0f };
    SetType(Type::Speed);

    m_pTexture = new Texture();
    if (FAILED(m_pTexture->Create(kEnemyTexture)))
    {
        MessageBox(NULL, "Texture load failed.\nEnemy.cpp", "Error", MB_OK);
    }
}

Enemy::~Enemy()
{
    if (m_pTexture)
    {
        delete m_pTexture;
        m_pTexture = nullptr;
    }
}

void Enemy::Update()
{
    const float stage = (m_stageSize > 0.0f) ? m_stageSize : kDefaultStageSize;
    const float half = stage * 0.5f;
    const float halfX = m_size.x * 0.5f;
    const float halfZ = m_size.z * 0.5f;

    float minX = -half + halfX;
    float maxX = half - halfX;
    float minZ = -half + halfZ;
    float maxZ = half - halfZ;
    if (minX > maxX) { minX = 0.0f; maxX = 0.0f; }
    if (minZ > maxZ) { minZ = 0.0f; maxZ = 0.0f; }

    const float chaseStart = stage * kChaseStartRatio;
    const float chaseEnd = stage * kChaseEndRatio;

    const float toTargetX = m_targetPos.x - m_pos.x;
    const float toTargetZ = m_targetPos.z - m_pos.z;
    const float targetDistSq = toTargetX * toTargetX + toTargetZ * toTargetZ;

    if (m_state == MoveState::Wander)
    {
        if (targetDistSq <= chaseStart * chaseStart)
        {
            m_state = MoveState::Chase;
        }
    }
    else
    {
        if (targetDistSq >= chaseEnd * chaseEnd)
        {
            m_state = MoveState::Wander;
            m_wanderTimer = 0.0f;
        }
    }

    if (m_state == MoveState::Chase)
    {
        const float dist = std::sqrt(targetDistSq);
        if (dist > 0.0001f)
        {
            float stopDist = (m_size.x > m_size.z) ? m_size.x : m_size.z;
            stopDist *= 0.6f;
            if (stopDist < kStopDistanceMin) stopDist = kStopDistanceMin;

            if (dist > stopDist)
            {
                float speedScale = 1.0f;
                if (dist < stopDist + kStopDistanceRange)
                {
                    speedScale = (dist - stopDist) / kStopDistanceRange;
                }
                m_pos.x += (toTargetX / dist) * m_moveSpeed * speedScale * kMoveDt;
                m_pos.z += (toTargetZ / dist) * m_moveSpeed * speedScale * kMoveDt;
            }
        }
    }
    else
    {
        m_wanderTimer -= kMoveDt;

        float toWanderX = m_wanderTarget.x - m_pos.x;
        float toWanderZ = m_wanderTarget.z - m_pos.z;
        float wanderDistSq = toWanderX * toWanderX + toWanderZ * toWanderZ;

        if (m_wanderTimer <= 0.0f || wanderDistSq <= kWanderReachEps * kWanderReachEps)
        {
            float targetX = (minX == maxX) ? minX : RandRange(minX, maxX);
            float targetZ = (minZ == maxZ) ? minZ : RandRange(minZ, maxZ);
            m_wanderTarget = { targetX, 0.0f, targetZ };
            m_wanderTimer = RandRange(kWanderTimerMin, kWanderTimerMax);

            toWanderX = m_wanderTarget.x - m_pos.x;
            toWanderZ = m_wanderTarget.z - m_pos.z;
            wanderDistSq = toWanderX * toWanderX + toWanderZ * toWanderZ;
        }

        const float dist = std::sqrt(wanderDistSq);
        if (dist > 0.0001f)
        {
            const float speed = m_moveSpeed * kWanderSpeedScale;
            m_pos.x += (toWanderX / dist) * speed * kMoveDt;
            m_pos.z += (toWanderZ / dist) * speed * kMoveDt;
        }
    }

    m_pos.x = ClampFloat(m_pos.x, minX, maxX);
    m_pos.z = ClampFloat(m_pos.z, minZ, maxZ);
    m_pos.y = 0.0f;
}

void Enemy::Draw()
{
    if (!m_pTexture) return;
    using namespace DirectX;
    // ---- ビルボード行列計算 ----
    XMMATRIX billboard = XMMatrixIdentity();

    if (m_pCamera)
    {
        // 転置していないカメラの View 行列を取得
        XMFLOAT4X4 viewFloat;
        //XMStoreFloat4x4(&viewFloat, m_pCamera->GetViewMatrix());
        viewFloat = m_pCamera->GetViewMatrix(false);

        // 読み取り用 → 計算用
        XMMATRIX viewMat = XMLoadFloat4x4(&viewFloat);

        // 逆行列（回転 + 移動を打ち消す）
        XMMATRIX invView = XMMatrixInverse(nullptr, viewMat);

        // 計算用 → 読み取り用
        XMFLOAT4X4 invViewFloat;
        XMStoreFloat4x4(&invViewFloat, invView);

        // 移動成分を削除（回転のみ残す）
        invViewFloat._41 = 0.0f;
        invViewFloat._42 = 0.0f;
        invViewFloat._43 = 0.0f;

        // 読み取り用 → 計算用
        billboard = XMLoadFloat4x4(&invViewFloat);
    }
    DirectX::XMMATRIX T = billboard * DirectX::XMMatrixTranslation(
        m_pos.x,
        m_pos.y + (m_size.y * 0.5f),
        m_pos.z
    );
    DirectX::XMFLOAT4X4 world;
    DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranspose(T));

    Sprite::SetWorld(world);
    Sprite::SetSize({ m_size.x, m_size.y });
    Sprite::SetOffset({ 0.0f, 0.0f });
    Sprite::SetUVPos({ 0.0f, 0.0f });
    Sprite::SetUVScale({ 1.0f, 1.0f });
    Sprite::SetColor(m_color);
    Sprite::SetTexture(m_pTexture);
    Sprite::Draw();
}

void Enemy::SetSize(const DirectX::XMFLOAT3& size)
{
    m_size = size;
}

void Enemy::SetStageSize(float size)
{
    m_stageSize = size;
}

void Enemy::SetMoveSpeed(float speed)
{
    m_moveSpeedBase = speed;
    m_moveSpeed = m_moveSpeedBase * m_moveSpeedScale;
}

void Enemy::SetType(Type type)
{
    m_type = type;
    switch (m_type)
    {
    case Type::Speed:
        m_maxHp = 2;
        m_hp = m_maxHp;
        m_moveSpeedScale = 1.45f;
        m_attackRangeScale = 0.95f;
        m_attackWindupScale = 0.75f;
        m_attackCooldownScale = 0.75f;
        m_attackDamageScale = 0.85f;
        m_color = { 1.0f, 0.50f, 0.25f, 1.0f };
        break;
    case Type::Tank:
        m_maxHp = 6;
        m_hp = m_maxHp;
        m_moveSpeedScale = 0.75f;
        m_attackRangeScale = 0.90f;
        m_attackWindupScale = 1.20f;
        m_attackCooldownScale = 1.10f;
        m_attackDamageScale = 1.40f;
        m_color = { 0.35f, 0.60f, 1.0f, 1.0f };
        break;
    case Type::Ranged:
    default:
        m_maxHp = 3;
        m_hp = m_maxHp;
        m_moveSpeedScale = 0.95f;
        m_attackRangeScale = 1.80f;
        m_attackWindupScale = 1.05f;
        m_attackCooldownScale = 1.25f;
        m_attackDamageScale = 0.75f;
        m_color = { 0.45f, 1.0f, 0.45f, 1.0f };
        break;
    }
    m_moveSpeed = m_moveSpeedBase * m_moveSpeedScale;
}

void Enemy::SetHpScale(float scale)
{
    if (scale < 0.1f) scale = 0.1f;

    const float hpRate = (m_maxHp > 0)
        ? static_cast<float>(m_hp) / static_cast<float>(m_maxHp)
        : 1.0f;
    const int scaledMaxHp = static_cast<int>(std::ceil(static_cast<float>(m_maxHp) * scale));
    m_maxHp = (scaledMaxHp > 0) ? scaledMaxHp : 1;
    m_hp = static_cast<int>(std::ceil(hpRate * static_cast<float>(m_maxHp)));
    if (m_hp < 0) m_hp = 0;
    if (m_hp > m_maxHp) m_hp = m_maxHp;
}

DirectX::XMFLOAT3 Enemy::GetSize() const
{
    return m_size;
}

Collision::Box Enemy::GetCollision() const
{
    Collision::Box box{};
    box.size = m_size;
    box.center = {
        m_pos.x,
        m_pos.y + m_size.y * 0.5f,
        m_pos.z
    };
    return box;
}

void Enemy::Damage(int amount)
{
    if (amount <= 0 || m_hp <= 0) return;
    m_hp -= amount;
    if (m_hp < 0) m_hp = 0;
}

bool Enemy::IsAlive() const
{
    return m_hp > 0;
}

int Enemy::GetHp() const
{
    return m_hp;
}

int Enemy::GetMaxHp() const
{
    return m_maxHp;
}

int Enemy::GetState() const
{
    return (m_state == MoveState::Chase) ? 1 : 0;
}

int Enemy::GetType() const
{
    return static_cast<int>(m_type);
}

float Enemy::GetAttackRangeScale() const
{
    return m_attackRangeScale;
}

float Enemy::GetAttackWindupScale() const
{
    return m_attackWindupScale;
}

float Enemy::GetAttackCooldownScale() const
{
    return m_attackCooldownScale;
}

float Enemy::GetAttackDamageScale() const
{
    return m_attackDamageScale;
}



void Enemy::SetCamera(Camera* camera)
{
    m_pCamera = camera;
}

void Enemy::SetTargetPos(DirectX::XMFLOAT3 get)
{
    m_targetPos = get;
}
