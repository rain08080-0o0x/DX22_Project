// Dice.cpp
#include "Dice.h"
// 必要なら描画用のヘッダも include する
 #include "Geometory.h"
// #include "Renderer.h"
#include "ShaderList.h"
#include "Sprite.h"
#include "Transfer.h"


#include <cmath>
#include <algorithm>
using namespace DirectX;

static XMVECTOR QuaternionFromTo(XMVECTOR from, XMVECTOR to);
// ローカル空間でのサイコロ各面の法線
static const DirectX::XMFLOAT3 FACE_NORMALS[6] =
{
    { 0,  1,  0}, // 上
    { 0, -1,  0}, // 下
    { 1,  0,  0}, // 右
    {-1,  0,  0}, // 左
    { 0,  0,  1}, // 前
    { 0,  0, -1}, // 後
};

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
    m_pModel = new Model();
    // 略可
    if (!m_pModel->Load("Assets/Model/Dice/dice.fbx", 1.0f,Model::ZFlip)) { // 倍率と反転は省略可
        MessageBox(NULL, "Not found Dice", "Error", MB_OK); // エラーメッセージの表示
    }

    m_pCamera = nullptr;
    TRAN_INS;
    tran.WallSize = { 5.0f,5.0f };

    m_angVel = { 0.0f,0.0f,0.0f };
    m_rot = { 0.0f,0.0f,0.0f,1.0f };
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

    m_angVel = { 0.0f,0.0f,0.0f };
    m_rot = { 0.0f,0.0f,0.0f,1.0f };
}

void Dice::Update(float dt)
{
    TRAN_INS;
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
    const float limitX = tran.WallSize.x;
    const float limitZ = tran.WallSize.y;

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
    
    // 接地判定（床処理した後でOK）
    m_onGround = (m_pos.y - half <= 0.0005f);

    // 角速度の減衰は「常に」かける（空中でも少し）
    const float airAngDamp = 0.15f;  // 空中：弱い
    const float groundAngDamp = 6.0f;   // 接地：強い
    const float damp = m_onGround ? groundAngDamp : airAngDamp;

    m_angVel.x -= m_angVel.x * damp * dt;
    m_angVel.y -= m_angVel.y * damp * dt;
    m_angVel.z -= m_angVel.z * damp * dt;

    // スリープ判定（一定フレーム連続で遅いなら止める）
    const float LIN_SLEEP = 0.10f;   // m/s
    const float ANG_SLEEP = 0.30f;   // rad/s
    const int   NEED_FRAMES = 20;    // 連続20フレームでスリープ（調整可）

    float v2 =
        m_vel.x * m_vel.x + m_vel.y * m_vel.y + m_vel.z * m_vel.z;
    float w2 =
        m_angVel.x * m_angVel.x + m_angVel.y * m_angVel.y + m_angVel.z * m_angVel.z;

    if (m_onGround && v2 < LIN_SLEEP * LIN_SLEEP && w2 < ANG_SLEEP * ANG_SLEEP)
    {
        m_sleepFrames++;
        if (m_sleepFrames >= NEED_FRAMES)
        {
            m_vel = DirectX::XMFLOAT3(0, 0, 0);
            m_angVel = DirectX::XMFLOAT3(0, 0, 0);
            m_sleepFrames = NEED_FRAMES; // 飽和


            SnapToGround();
        }
    }
    else
    {
        m_sleepFrames = 0;
    }

    // 回転更新（角速度が十分ある時だけ）
    const float wx = m_angVel.x, wy = m_angVel.y, wz = m_angVel.z;
    const float wlen = sqrt(wx * wx + wy * wy + wz * wz);

    if (wlen > 0.0001f)
    {
        using namespace DirectX;
        XMVECTOR q = XMLoadFloat4(&m_rot);

        XMVECTOR axis = XMVectorSet(wx / wlen, wy / wlen, wz / wlen, 0.0f);
        float angle = wlen * dt; // rad

        XMVECTOR dq = XMQuaternionRotationAxis(axis, angle);
        q = XMQuaternionNormalize(XMQuaternionMultiply(dq, q));

        XMStoreFloat4(&m_rot, q);
    }

}

void Dice::Draw()
{
    XMVECTOR q = XMLoadFloat4(&m_rot);

    XMMATRIX S = XMMatrixScaling(m_size, m_size, m_size);
    XMMATRIX R = XMMatrixRotationQuaternion(q);
    XMMATRIX T = XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);

    XMMATRIX W = S * R * T;

    XMFLOAT4X4 world;
    XMStoreFloat4x4(&world, XMMatrixTranspose(W));

    // Geometory::SetWorld(world);
    // Geometory::DrawBox();


    XMFLOAT4X4 fWVP[3];

    fWVP[0] = world;
    fWVP[1] = m_pCamera->GetViewMatrix();
    fWVP[2] = m_pCamera->GetProjectionMatrix();

	// シェーダーへ変換行列を設定 
    ShaderList::SetWVP(fWVP); // SetWVP関数の引数にはXMFLOAT4X4型で要素数３の配列のアドレスを渡す 
    if(false)
    {
        Geometory::SetView(m_pCamera->GetViewMatrix(true));
        Geometory::SetProjection(m_pCamera->GetProjectionMatrix(true));

        // Spriteへの設定
        Sprite::SetView(m_pCamera->GetViewMatrix(true));
        Sprite::SetProjection(m_pCamera->GetProjectionMatrix(true));
    }
    if(true)
    {
        //Geometory::SetView(fWVP[1]);
        //Geometory::SetProjection(fWVP[2]);
        Sprite::SetView(fWVP[1]);
        Sprite::SetProjection(fWVP[2]);
    }

    //　モデルに使用する頂点シェーダー、ピクセルシェーダーを設定
    m_pModel->SetVertexShader(ShaderList::GetVS(ShaderList::VS_WORLD));
    m_pModel->SetPixelShader(ShaderList::GetPS(ShaderList::PS_LAMBERT));

    // マテリアル別にメッシュを表示 
    for (unsigned int i = 0; i < m_pModel->GetMeshNum(); ++i)
    {
        // モデルのメッシュを取得 
        const Model::Mesh* mesh = m_pModel->GetMesh(i);
        // メッシュに割り当てられているマテリアルを取得 
        Model::Material material = *m_pModel->GetMaterial(mesh->materialID);
        // シェーダーへマテリアルを設定 
        ShaderList::SetMaterial(material);
        // モデルの描画 
        m_pModel->Draw(i);
    }
}

void Dice::Uninit()
{
    // 今は特に解放するものはないが、将来テクスチャやモデルを持たせるならここで解放
}

void Dice::SetCamera(Camera *set)
{
    m_pCamera = set;
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

void Dice::SnapToGround()
{
    using namespace DirectX;

    XMVECTOR q = XMLoadFloat4(&m_rot);
    XMMATRIX R = XMMatrixRotationQuaternion(q);

    static const XMFLOAT3 FACE_NORMALS[6] =
    {
        { 0,  1,  0}, // 上
        { 0, -1,  0}, // 下
        { 1,  0,  0}, // 右
        {-1,  0,  0}, // 左
        { 0,  0,  1}, // 前
        { 0,  0, -1}, // 後
    };

    int bestFace = -1;
    float bestDot = -1.0f;

    XMVECTOR down = XMVectorSet(0, -1, 0, 0);

    for (int i = 0; i < 6; ++i)
    {
        XMVECTOR n = XMLoadFloat3(&FACE_NORMALS[i]);
        XMVECTOR wn = XMVector3Normalize(
            XMVector3TransformNormal(n, R)
        );

        float dot = XMVectorGetX(XMVector3Dot(wn, down));
        if (dot > bestDot)
        {
            bestDot = dot;
            bestFace = i;
        }
    }

    if (bestFace < 0) return;

    XMVECTOR from = XMVector3Normalize(
        XMLoadFloat3(&FACE_NORMALS[bestFace])
    );
    XMVECTOR to = down;

    // ★ 正しい関数
    XMVECTOR snapQ = QuaternionFromTo(from, to);
    snapQ = XMQuaternionNormalize(snapQ);

    q = XMQuaternionMultiply(snapQ, q);
    q = XMQuaternionNormalize(q);

    XMStoreFloat4(&m_rot, q);
}

static XMVECTOR QuaternionFromTo(XMVECTOR from, XMVECTOR to)
{
    from = XMVector3Normalize(from);
    to = XMVector3Normalize(to);

    float dot = XMVectorGetX(XMVector3Dot(from, to));

    // ほぼ同じ向き → 回転なし
    if (dot > 0.9999f)
    {
        return XMQuaternionIdentity();
    }

    // ほぼ逆向き → 180度回転（軸を適当に選ぶ必要あり）
    if (dot < -0.9999f)
    {
        // from と直交する軸を作る
        XMVECTOR axis = XMVector3Cross(from, XMVectorSet(1, 0, 0, 0));
        if (XMVectorGetX(XMVector3LengthSq(axis)) < 1e-6f)
        {
            axis = XMVector3Cross(from, XMVectorSet(0, 1, 0, 0));
        }
        axis = XMVector3Normalize(axis);
        return XMQuaternionRotationAxis(axis, XM_PI);
    }

    // 通常ケース
    XMVECTOR axis = XMVector3Cross(from, to);

    // q = [axis, 1 + dot] を正規化
    XMVECTOR q = XMVectorSet(
        XMVectorGetX(axis),
        XMVectorGetY(axis),
        XMVectorGetZ(axis),
        1.0f + dot
    );

    return XMQuaternionNormalize(q);
}
