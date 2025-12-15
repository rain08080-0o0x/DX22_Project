// Dice.cpp
#include "Dice.h"
// 必要なら描画用のヘッダも include する
 #include "Geometory.h"
// #include "Renderer.h"
#include "ShaderList.h"
#include "Sprite.h"
#include "Transfer.h"

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
    m_pModel = new Model();
    // 略可
    if (!m_pModel->Load("Assets/Model/Dice/dice.fbx", 1.0f,Model::ZFlip)) { // 倍率と反転は省略可
        MessageBox(NULL, "Not found Dice", "Error", MB_OK); // エラーメッセージの表示
    }

    m_pCamera = nullptr;
    TRAN_INS;
    tran.WallSize = { 10.0f,10.0f };

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
    //Geometory::DrawBox();  // 立方体を描画する関数

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

