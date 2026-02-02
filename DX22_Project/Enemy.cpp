#include "Enemy.h"

namespace
{
    const char* kEnemyTexture = "Assets/Texture/Chracter/genbaneko.png";
}

Enemy::Enemy()
    : m_pTexture(nullptr)
    , m_size(0.5f, 1.0f, 0.5f)
    , m_color(1.0f, 0.25f, 0.25f, 1.0f)
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
    // Stay still for now.
}

void Enemy::Draw()
{
    if (!m_pTexture) return;

    DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
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
