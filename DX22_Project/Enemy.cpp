#include "Enemy.h"

namespace
{
    const char* kEnemyTexture = "Assets/Texture/Chracter/genbaneko.png";
    const int kDefaultEnemyHp = 3;
    const float kDefaultStageSize = 5.0f;
    const float kDefaultMoveSpeed = 1.2f;
    const float kMoveDt = 1.0f / 60.0f;
}

Enemy::Enemy()
    : m_pTexture(nullptr)
    , m_size(0.5f, 1.0f, 0.5f)
    , m_color(1.0f, 0.25f, 0.25f, 1.0f)
    , m_moveSpeed(kDefaultMoveSpeed)
    , m_stageSize(kDefaultStageSize)
    , m_moveDirX(1.0f)
    , m_hp(kDefaultEnemyHp)
    , m_maxHp(kDefaultEnemyHp)
{
    m_pos = { 0.0f, 0.0f, 0.0f };

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

    float minX = -half + halfX;
    float maxX = half - halfX;
    if (minX > maxX)
    {
        minX = 0.0f;
        maxX = 0.0f;
    }

    m_pos.x += m_moveDirX * m_moveSpeed * kMoveDt;

    if (m_pos.x <= minX)
    {
        m_pos.x = minX;
        m_moveDirX = 1.0f;
    }
    else if (m_pos.x >= maxX)
    {
        m_pos.x = maxX;
        m_moveDirX = -1.0f;
    }

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
    m_moveSpeed = speed;
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


void Enemy::SetCamera(Camera* camera)
{
    m_pCamera = camera;
}
