#include "SceneGame.h"
#include "Enemy.h"
#include "CameraDebug.h"
#include "Geometory.h"
#include "Sprite.h"
#include "DirectX.h"
#include "Transfer.h"
#include "UIObject.h"
#include "Collision.h"

namespace
{
    const float kUiMargin = 20.0f;
    const float kHpFrameWidth = 260.0f;
    const float kHpFrameHeight = 32.0f;
    const float kHpGaugePadding = 4.0f;

    float Clamp01(float v)
    {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    }
}

SceneGame::SceneGame()
    : m_pCamera(nullptr)
    , m_pPlayer(nullptr)
    , m_pEnemy(nullptr)
    , m_enemyWasOverlapping(false)
    , m_pHpFrame(nullptr)
    , m_pHpGauge(nullptr)
    , m_stageSize(5.0f)
{
    m_pCamera = new CameraDebug();
    if (m_pCamera)
    {
        m_pCamera->LockPos(true);
        TRAN_INS;
        tran.camera.eye = { 0.0f, 6.0f, -6.0f };
        tran.camera.look = { 0.0f, 0.0f, 0.0f };
        m_pCamera->SetPos(tran.camera.eye);
        m_pCamera->SetLook(tran.camera.look);
    }

    m_pPlayer = new Player();
    {
        TRAN_INS;
        if (tran.player.stageSize <= 0.0f)
        {
            tran.player.stageSize = m_stageSize;
        }
    }

    m_pEnemy = new Enemy();
    if (m_pEnemy)
    {
        TRAN_INS;
        m_pEnemy->SetSize(tran.player.size);
        m_pEnemy->SetPos({ 2.0f, 0.0f, 0.0f });
    }

    const float frameX = kUiMargin + kHpFrameWidth * 0.5f;
    const float frameY = kUiMargin + kHpFrameHeight * 0.5f;
    m_pHpFrame = new UIObject("UIFrame.png", frameX, frameY, kHpFrameWidth, kHpFrameHeight);

    const float gaugeWidth = kHpFrameWidth - kHpGaugePadding * 2.0f;
    const float gaugeHeight = kHpFrameHeight - kHpGaugePadding * 2.0f;
    const float gaugeLeft = kUiMargin + kHpGaugePadding;
    const float gaugeX = gaugeLeft + gaugeWidth * 0.5f;
    const float gaugeY = frameY;
    m_pHpGauge = new UIObject("UIGauge.png", gaugeX, gaugeY, gaugeWidth, gaugeHeight);

    m_uiManager.Add(m_pHpGauge, UIObjectManager::Layer::Game);
    m_uiManager.Add(m_pHpFrame, UIObjectManager::Layer::Game);

    UpdateHpGauge();
}

SceneGame::~SceneGame()
{
    m_uiManager.ClearAll();

    if (m_pHpGauge)
    {
        delete m_pHpGauge;
        m_pHpGauge = nullptr;
    }
    if (m_pHpFrame)
    {
        delete m_pHpFrame;
        m_pHpFrame = nullptr;
    }
    if (m_pEnemy)
    {
        delete m_pEnemy;
        m_pEnemy = nullptr;
    }
    if (m_pPlayer)
    {
        delete m_pPlayer;
        m_pPlayer = nullptr;
    }
    if (m_pCamera)
    {
        delete m_pCamera;
        m_pCamera = nullptr;
    }
}

void SceneGame::Update()
{
    if (m_pCamera) m_pCamera->Update();
    if (m_pPlayer) m_pPlayer->Update();
    if (m_pEnemy) m_pEnemy->Update();

    if (m_pEnemy)
    {
        TRAN_INS;
        m_pEnemy->SetSize(tran.player.size);
    }

    if (m_pPlayer && m_pEnemy)
    {
        TRAN_INS;

        Collision::Box playerBox{};
        playerBox.size = tran.player.size;
        playerBox.center = {
            tran.player.pos.x,
            tran.player.pos.y + tran.player.size.y * 0.5f,
            tran.player.pos.z
        };

        Collision::Box enemyBox = m_pEnemy->GetCollision();
        const bool isHit = Collision::Hit(playerBox, enemyBox).isHit;
        if (isHit && !m_enemyWasOverlapping)
        {
            tran.player.hp -= 1.0f;
            if (tran.player.hp < 0.0f) tran.player.hp = 0.0f;
        }
        m_enemyWasOverlapping = isHit;
    }


    UpdateHpGauge();
    m_uiManager.Update(UIObjectManager::Layer::Game);
}

void SceneGame::Draw()
{
    if (!m_pCamera) return;

    DirectX::XMFLOAT4X4 view = m_pCamera->GetViewMatrix();
    DirectX::XMFLOAT4X4 proj = m_pCamera->GetProjectionMatrix();

    Geometory::SetView(view);
    Geometory::SetProjection(proj);
    Sprite::SetView(view);
    Sprite::SetProjection(proj);

    SetDepthTest(true);

    float stage = m_stageSize;
    {
        TRAN_INS;
        if (tran.player.stageSize > 0.0f) stage = tran.player.stageSize;
    }

    DirectX::XMMATRIX S = DirectX::XMMatrixScaling(stage, 0.1f, stage);
    DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(0.0f, -0.05f, 0.0f);
    DirectX::XMFLOAT4X4 world;
    DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranspose(S * T));
    Geometory::SetWorld(world);
    Geometory::DrawBox();

    if (m_pEnemy) m_pEnemy->Draw();
    if (m_pPlayer) m_pPlayer->Draw();
    m_uiManager.Draw(UIObjectManager::Layer::Game);
}

void SceneGame::UpdateHpGauge()
{
    if (!m_pHpGauge || !m_pHpFrame) return;

    TRAN_INS;
    float maxHp = tran.player.maxHp;
    float hp = tran.player.hp;
    if (maxHp <= 0.0f) maxHp = 1.0f;

    const float rate = Clamp01(hp / maxHp);
    const float gaugeWidth = kHpFrameWidth - kHpGaugePadding * 2.0f;
    const float gaugeHeight = kHpFrameHeight - kHpGaugePadding * 2.0f;
    const float gaugeLeft = kUiMargin + kHpGaugePadding;
    const float gaugeX = gaugeLeft + gaugeWidth * rate * 0.5f;
    const float gaugeY = kUiMargin + kHpFrameHeight * 0.5f;

    m_pHpGauge->SetPosition(gaugeX, gaugeY);
    m_pHpGauge->SetSize(gaugeWidth * rate, gaugeHeight);
    m_pHpGauge->SetUVPosition(0.0f, 0.0f);
    m_pHpGauge->SetUVScale(rate, 1.0f);
}
