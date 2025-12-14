// Dice.cpp
#include "Dice.h"
// 必要なら描画用のヘッダも include する
 #include "Geometory.h"
// #include "Renderer.h"

using namespace DirectX;

Dice::Dice()
    : m_pos(0.0f, 0.0f, 0.0f)
    , m_vel(0.0f, 0.0f, 0.0f)
    , m_size(1.0f)
    , m_mass(1.0f)
    , m_restitution(0.4f)
    , m_friction(2.0f) // 大きいほどすぐ減速
{
    m_box.center = m_pos;
    m_box.size = XMFLOAT3(m_size, m_size, m_size);
}

void Dice::Init(const XMFLOAT3& pos, float size)
{
    m_pos = pos;
    m_vel = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_size = size;

    m_mass = 1.0f;
    m_restitution = 0.4f; // 床でのバウンドの強さ
    m_friction = 2.0f; // 床摩擦

    m_box.center = m_pos;
    m_box.size = XMFLOAT3(m_size, m_size, m_size);
}

void Dice::Update(float dt)
{
    // 1. 重力
    const float gravity = -9.8f;
    m_vel.y += gravity * dt;

    // 2. 速度で位置を更新
    m_pos.x += m_vel.x * dt;
    m_pos.y += m_vel.y * dt;
    m_pos.z += m_vel.z * dt;

    const float half = m_size * 0.5f;

    // 3. 床との衝突 (y = 0 を床とする)
    if (m_pos.y - half < 0.0f)
    {
        m_pos.y = half;                       // めり込み修正
        m_vel.y = -m_vel.y * m_restitution;   // 反発

        // 床との摩擦で x,z を減速（雑だけど分かりやすい形）
        m_vel.x -= m_vel.x * m_friction * dt;
        m_vel.z -= m_vel.z * m_friction * dt;
    }

    // 4. 壁との衝突 (簡易的に x,z の範囲を決める)
    const float limitX = 10.0f;
    const float limitZ = 10.0f;

    if (m_pos.x - half < -limitX)
    {
        m_pos.x = -limitX + half;
        m_vel.x = -m_vel.x * m_restitution;
    }
    else if (m_pos.x + half > limitX)
    {
        m_pos.x = limitX - half;
        m_vel.x = -m_vel.x * m_restitution;
    }

    if (m_pos.z - half < -limitZ)
    {
        m_pos.z = -limitZ + half;
        m_vel.z = -m_vel.z * m_restitution;
    }
    else if (m_pos.z + half > limitZ)
    {
        m_pos.z = limitZ - half;
        m_vel.z = -m_vel.z * m_restitution;
    }

    // 5. 当たり判定の center を位置に同期
    m_box.center = m_pos;
    // size は変わらないのでそのまま
}

void Dice::Draw()
{
    // あなたの環境の描画パイプラインに合わせて書き換え

    XMMATRIX S = XMMatrixScaling(m_size, m_size, m_size);
    XMMATRIX T = XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);
    XMMATRIX W = S * T;

    XMFLOAT4X4 world;
    XMStoreFloat4x4(&world, XMMatrixTranspose(W));

    Geometory::SetWorld(world);
    Geometory::DrawBox();  // 立方体を描画する関数
}

void Dice::Uninit()
{
    // 今は特に解放するものはないが、将来テクスチャやモデルを持たせるならここで解放
}

const DirectX::XMFLOAT3 Dice::GetPos()
{
    return m_pos;
}

const DirectX::XMFLOAT3 Dice::GetVel()
{ 
    return m_vel; 
}

void Dice::SetVel(const DirectX::XMFLOAT3 v)
{ 
    m_vel = v;
}

void Dice::AddPos(const DirectX::XMFLOAT3 dp)
{
    m_pos.x += dp.x; 
    m_pos.y += dp.y; 
    m_pos.z += dp.z;
    m_box.center = m_pos; // 追従
}