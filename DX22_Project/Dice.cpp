// Dice.cpp
#include "Dice.h"
#include <cmath>
#include "ShaderList.h"
#include "Sprite.h"
#include "Texture.h"
#include "Transfer.h"
using namespace DirectX;

Dice::Dice()
    : m_pos(0, 0, 0)
    , m_vel(0, 0, 0)
    , m_rot(0, 0, 0, 1)
    , m_angVel(0, 0, 0)
    , m_size(1.0f)
    , m_mass(1.0f)
    , m_restitution(0.4f)   // 弾み具合
    , m_linearDamping(0.1f) // 空中抵抗
    , m_friction(3.0f)      // 床摩擦
    , m_pCamera(nullptr)
{
    // AABB 初期化
    m_box.center = m_pos;
    m_box.size = XMFLOAT3(m_size, m_size, m_size);

    m_pModel = new Model();
    if (!m_pModel->Load("Assets/Model/Dice/dice.fbx")) 
    { // 倍率と反転は省略可
        MessageBox(NULL, "Not Found", "Error", MB_OK); // エラーメッセージの表示
    }
    TRAN_INS;
    tran.WallSize.x = 5.0f;
    tran.WallSize.y = 5.0f;
    WALL_LIMIT_Y = 5.0f;
}

void Dice::Init(const XMFLOAT3& pos, float size)
{
    m_pos = pos;
    m_vel = XMFLOAT3(0, 0, 0);

    // 初期回転は「無回転」から始める（RollAllでランダムにしてもOK）
    m_rot = XMFLOAT4(0, 0, 0, 1);
    m_angVel = XMFLOAT3(0, 0, 0);

    m_size = size;

    // AABB を位置に同期
    m_box.center = m_pos;
    m_box.size = XMFLOAT3(m_size, m_size, m_size);
}
void Dice::Update(float dt)
{
    TRAN_INS;
    WALL_LIMIT_X = tran.WallSize.x;
    WALL_LIMIT_Z = tran.WallSize.y;
    if (m_sleeping)
    {
        // 位置は固定、当たり判定だけ同期
        m_box.center = m_pos;
        return;
    }
    const float half = m_size * 0.5f;

    // ---------- 1) 重力 ----------
    m_vel.y += GRAVITY * dt;

    // ---------- 2) 移動 ----------
    m_pos.x += m_vel.x * dt;
    m_pos.y += m_vel.y * dt;
    m_pos.z += m_vel.z * dt;

    // ---------- 3) 床との衝突 ----------
    if (m_pos.y < half)
    {
        m_pos.y = half;

        if (m_vel.y < 0.0f)
            m_vel.y = -m_vel.y * m_restitution;

        // 接地時の摩擦
        m_vel.x -= m_vel.x * m_friction * dt;
        m_vel.z -= m_vel.z * m_friction * dt;
    }

    // ---------- 4) 壁との衝突（X方向） ----------
    if (m_pos.x < -WALL_LIMIT_X + half)
    {
        m_pos.x = -WALL_LIMIT_X + half;
        m_vel.x = -m_vel.x * m_restitution;
    }
    else if (m_pos.x > WALL_LIMIT_X - half)
    {
        m_pos.x = WALL_LIMIT_X - half;
        m_vel.x = -m_vel.x * m_restitution;
    }

    // ---------- 5) 壁との衝突（Z方向） ----------
    if (m_pos.z < -WALL_LIMIT_Z + half)
    {
        m_pos.z = -WALL_LIMIT_Z + half;
        m_vel.z = -m_vel.z * m_restitution;
    }
    else if (m_pos.z > WALL_LIMIT_Z - half)
    {
        m_pos.z = WALL_LIMIT_Z - half;
        m_vel.z = -m_vel.z * m_restitution;
    }

    // ---------- 6) 空気抵抗 ----------
    m_vel.x -= m_vel.x * m_linearDamping * dt;
    m_vel.y -= m_vel.y * m_linearDamping * dt;
    m_vel.z -= m_vel.z * m_linearDamping * dt;

    m_angVel.x *= 0.96f;
    m_angVel.y *= 0.96f;
    m_angVel.z *= 0.96f;
    // ---------- 7) 回転 ----------
    IntegrateRotation(dt);

    // ---------- 8) 当たり判定同期 ----------
    m_box.center = m_pos;

    // ---------- 9) スリープ判定（止まる処理） ----------
    const float LIN_SLEEP = 0.05f;  // m/s
    const float ANG_SLEEP = 0.10f;  // rad/s
    const int   NEED_FRAMES = 30;   // 0.5秒くらい（60fps想定）

    float v2 =
        m_vel.x * m_vel.x + m_vel.y * m_vel.y + m_vel.z * m_vel.z;
    float w2 =
        m_angVel.x * m_angVel.x + m_angVel.y * m_angVel.y + m_angVel.z * m_angVel.z;

    bool onGround = (m_pos.y <= m_size * 0.5f + 0.001f);

    if (onGround && v2 < LIN_SLEEP * LIN_SLEEP && w2 < ANG_SLEEP * ANG_SLEEP)
    {
        m_sleepFrames++;
        if (m_sleepFrames >= NEED_FRAMES)
        {
            m_sleeping = true;
            m_vel = { 0,0,0 };
            m_angVel = { 0,0,0 };
        }
    }
    else
    {
        m_sleepFrames = 0;
        //m_sleeping = false;
    }

}


void Dice::IntegrateRotation(float dt)
{
    // 角速度ベクトル w = (wx, wy, wz)
    // |w| を角速度の大きさとして、軸 = w/|w|、角度 = |w|*dt でクォータニオン更新
    const float wx = m_angVel.x;
    const float wy = m_angVel.y;
    const float wz = m_angVel.z;

    const float wlen = std::sqrt(wx * wx + wy * wy + wz * wz);
    if (wlen < 1e-6f)
        return; // 角速度ゼロなら回転しない

    XMVECTOR q = XMLoadFloat4(&m_rot);

    XMVECTOR axis = XMVectorSet(wx / wlen, wy / wlen, wz / wlen, 0.0f);
    const float angle = wlen * dt;

    // dt分だけ回す回転dqを作る
    XMVECTOR dq = XMQuaternionRotationAxis(axis, angle);

    // dq * q で姿勢更新（左掛け）
    q = XMQuaternionNormalize(XMQuaternionMultiply(dq, q));

    XMStoreFloat4(&m_rot, q);
}

void Dice::Draw()
{
    DirectX::XMFLOAT4X4 fWVP[3];
    // あなたの描画システムに合わせて SetWorld/DrawBox を呼ぶ想定

    XMVECTOR q = XMLoadFloat4(&m_rot);

    XMMATRIX S = XMMatrixScaling(m_size, m_size, m_size);
    XMMATRIX R = XMMatrixRotationQuaternion(q);
    XMMATRIX T = XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);

    XMMATRIX W = S * R * T;

    XMFLOAT4X4 world;
    XMStoreFloat4x4(&world, XMMatrixTranspose(W));

    fWVP[0] = world;
    fWVP[1] = m_pCamera->GetViewMatrix(true);
    fWVP[2] = m_pCamera->GetProjectionMatrix(true);

    // シェーダーへ変換行列を設定 
    ShaderList::SetWVP(fWVP); // SetWVP関数の引数にはXMFLOAT4X4型で要素数３の配列のアドレスを渡す 

    // モデルに使用する頂点シェーダー、ピクセルシェーダーを設定 
    m_pModel->SetVertexShader(ShaderList::GetVS(ShaderList::VS_WORLD));
    m_pModel->SetPixelShader(ShaderList::GetPS(ShaderList::PS_LAMBERT));

    for (unsigned int i = 0; i < m_pModel->GetMeshNum(); ++i) {
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

Collision::OBB Dice::GetOBB()
{
    using namespace DirectX;

    Collision::OBB obb;

    // ---- 中心 ----
    obb.center = m_pos;

    // ---- 半サイズ ----
    obb.halfSize =
    {
        m_size * 0.5f,
        m_size * 0.5f,
        m_size * 0.5f
    };

    // ---- 回転行列を作る ----
    XMMATRIX R = XMMatrixRotationQuaternion(
        XMLoadFloat4(&m_rot)
    );

    // 3x3 行列として取り出す
    XMFLOAT3X3 m;
    XMStoreFloat3x3(&m, R);

    // ---- 回転後のローカル軸 ----
    // 行列の「行」をそのまま使う
    obb.axis[0] = { m._11, m._12, m._13 }; // X軸
    obb.axis[1] = { m._21, m._22, m._23 }; // Y軸
    obb.axis[2] = { m._31, m._32, m._33 }; // Z軸

    // 念のため正規化（DirectXMath的にはほぼ不要だが安全）
    for (int i = 0; i < 3; ++i)
    {
        XMVECTOR a = XMLoadFloat3(&obb.axis[i]);
        a = XMVector3Normalize(a);
        XMStoreFloat3(&obb.axis[i], a);
    }

    return obb;
}

void Dice::WakeUp()
{
    m_sleeping = false;
    m_sleepFrames = 0;
}
