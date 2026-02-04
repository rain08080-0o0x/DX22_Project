#include "Goal.h"
#include "Sprite.h"
#include <Windows.h>

using namespace DirectX;

Goal::Goal(XMFLOAT3 size)
    : m_pGoalTex(nullptr)
    , m_pCamera(nullptr)
{
    m_pGoalTex = new Texture();
    if (FAILED(m_pGoalTex->Create("Assets/Texture/GoalTex.png")))
    {
        MessageBox(nullptr, "Texture load failed.\nGoal.cpp", "Error", MB_OK);
    }

    m_scale = size;
}

Goal::~Goal()
{
    delete m_pGoalTex;
    m_pGoalTex = nullptr;
}

void Goal::Update()
{
    /* 例：上下にふわふわさせる簡易アニメーション*/static float t = 0.0f;t += 0.05f;m_pos.y += sinf(t) * 0.01f;
}

void Goal::Draw()
{
    // ---- スプライト用 View / Projection 設定 ----
    Sprite::SetView(m_pCamera->GetViewMatrix());
    Sprite::SetProjection(m_pCamera->GetProjectionMatrix());

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

    // ---- ワールド行列 ----
    XMMATRIX world =
        billboard *
        XMMatrixScaling(m_scale.x, m_scale.y, 1.0f) *
        XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);

    // 転置して Sprite 用に渡す
    XMFLOAT4X4 worldFloat;
    XMStoreFloat4x4(&worldFloat, XMMatrixTranspose(world));

    // ---- スプライト描画 ----
    Sprite::SetColor({ 1,1,1,1 });
    Sprite::SetOffset({ 0,0 });
    Sprite::SetWorld(worldFloat);
    Sprite::SetSize({ 1.0f, 1.0f });
    Sprite::SetUVPos({ 0,0 });
    Sprite::SetUVScale({ 1,1 });
    Sprite::SetTexture(m_pGoalTex);
    Sprite::Draw();
}

void Goal::SetCamera(Camera* camera)
{
    m_pCamera = camera;
}
