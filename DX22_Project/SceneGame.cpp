#include "SceneGame.h"
#include "Enemy.h"
#include "CameraDebug.h"
#include "Geometory.h"
#include "Sprite.h"
#include "DirectX.h"
#include "Input.h"
#include "Defines.h"
#include "Transfer.h"
#include "UIObject.h"
#include "Collision.h"
#include "Texture.h"
#include <cmath>

namespace
{
    const float kUiMargin = 20.0f;
    const float kHpFrameWidth = 260.0f;
    const float kHpFrameHeight = 32.0f;
    const float kHpGaugePadding = 4.0f;
    const float kAttackDuration = 0.15f;
    const float kFixedDt = 1.0f / 60.0f;
    const float kEnemyHpBillboardWidthScale = 2.0f;
    const float kEnemyHpBillboardHeightScale = 0.2f;
    const float kEnemyHpBillboardPaddingScale = 0.2f;
    const float kEnemyHpBillboardOffsetScale = 0.7f;
    const float kEnemyHpBillboardMinWidth = 0.6f;
    const float kEnemyHpBillboardMinHeight = 0.1f;

    const int kCameraModeGame = 0;
    const int kCameraModeDebug = 1;

    int NormalizeCameraMode(int mode)
    {
        return (mode == kCameraModeDebug) ? kCameraModeDebug : kCameraModeGame;
    }

    void ApplyCameraPose(CameraDebug* camera, const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& look)
    {
        if (camera)
        {
            camera->SetPose(eye, look);
        }
    }


    float Clamp01(float v)
    {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    }

    void AddAabbLines(const Collision::Box& box, const DirectX::XMFLOAT4& color)
    {
        const float hx = box.size.x * 0.5f;
        const float hy = box.size.y * 0.5f;
        const float hz = box.size.z * 0.5f;
        const DirectX::XMFLOAT3 c = box.center;

        DirectX::XMFLOAT3 v[8] =
        {
            {c.x - hx, c.y - hy, c.z - hz},
            {c.x + hx, c.y - hy, c.z - hz},
            {c.x - hx, c.y + hy, c.z - hz},
            {c.x + hx, c.y + hy, c.z - hz},
            {c.x - hx, c.y - hy, c.z + hz},
            {c.x + hx, c.y - hy, c.z + hz},
            {c.x - hx, c.y + hy, c.z + hz},
            {c.x + hx, c.y + hy, c.z + hz},
        };

        Geometory::AddLine(v[0], v[1], color);
        Geometory::AddLine(v[1], v[3], color);
        Geometory::AddLine(v[3], v[2], color);
        Geometory::AddLine(v[2], v[0], color);

        Geometory::AddLine(v[4], v[5], color);
        Geometory::AddLine(v[5], v[7], color);
        Geometory::AddLine(v[7], v[6], color);
        Geometory::AddLine(v[6], v[4], color);

        Geometory::AddLine(v[0], v[4], color);
        Geometory::AddLine(v[1], v[5], color);
        Geometory::AddLine(v[2], v[6], color);
        Geometory::AddLine(v[3], v[7], color);
    }

    void AddCameraFrustumLines(const Camera& camera, const DirectX::XMFLOAT4& color)
    {
        using namespace DirectX;

        const float nearZ = camera.GetNear();
        const float farZ = camera.GetFar();
        if (nearZ <= 0.0f || farZ <= 0.0f || farZ <= nearZ)
        {
            return;
        }

        const float fovy = camera.GetFovy();
        const float aspect = camera.GetAspect();

        XMFLOAT3 pos = camera.GetPos();
        XMFLOAT3 look = camera.GetLook();
        XMFLOAT3 up = camera.GetUp();

        XMVECTOR vPos = XMLoadFloat3(&pos);
        XMVECTOR vLook = XMLoadFloat3(&look);
        XMVECTOR vUp = XMLoadFloat3(&up);

        XMVECTOR forward = vLook - vPos;
        if (XMVectorGetX(XMVector3LengthSq(forward)) < 1.0e-6f)
        {
            forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        }
        else
        {
            forward = XMVector3Normalize(forward);
        }

        if (XMVectorGetX(XMVector3LengthSq(vUp)) < 1.0e-6f)
        {
            vUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        }
        else
        {
            vUp = XMVector3Normalize(vUp);
        }

        XMVECTOR right = XMVector3Cross(vUp, forward);
        if (XMVectorGetX(XMVector3LengthSq(right)) < 1.0e-6f)
        {
            right = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
        }
        else
        {
            right = XMVector3Normalize(right);
        }

        vUp = XMVector3Normalize(XMVector3Cross(forward, right));

        const float tanHalfFovy = tanf(fovy * 0.5f);
        const float nearH = tanHalfFovy * nearZ;
        const float nearW = nearH * aspect;
        const float farH = tanHalfFovy * farZ;
        const float farW = farH * aspect;

        XMVECTOR nearCenter = vPos + forward * nearZ;
        XMVECTOR farCenter = vPos + forward * farZ;

        XMVECTOR upNear = vUp * nearH;
        XMVECTOR rightNear = right * nearW;
        XMVECTOR upFar = vUp * farH;
        XMVECTOR rightFar = right * farW;

        XMVECTOR ntl = nearCenter + upNear - rightNear;
        XMVECTOR ntr = nearCenter + upNear + rightNear;
        XMVECTOR nbl = nearCenter - upNear - rightNear;
        XMVECTOR nbr = nearCenter - upNear + rightNear;

        XMVECTOR ftl = farCenter + upFar - rightFar;
        XMVECTOR ftr = farCenter + upFar + rightFar;
        XMVECTOR fbl = farCenter - upFar - rightFar;
        XMVECTOR fbr = farCenter - upFar + rightFar;

        XMFLOAT3 v[8];
        XMStoreFloat3(&v[0], ntl);
        XMStoreFloat3(&v[1], ntr);
        XMStoreFloat3(&v[2], nbr);
        XMStoreFloat3(&v[3], nbl);
        XMStoreFloat3(&v[4], ftl);
        XMStoreFloat3(&v[5], ftr);
        XMStoreFloat3(&v[6], fbr);
        XMStoreFloat3(&v[7], fbl);

        Geometory::AddLine(v[0], v[1], color);
        Geometory::AddLine(v[1], v[2], color);
        Geometory::AddLine(v[2], v[3], color);
        Geometory::AddLine(v[3], v[0], color);

        Geometory::AddLine(v[4], v[5], color);
        Geometory::AddLine(v[5], v[6], color);
        Geometory::AddLine(v[6], v[7], color);
        Geometory::AddLine(v[7], v[4], color);

        Geometory::AddLine(v[0], v[4], color);
        Geometory::AddLine(v[1], v[5], color);
        Geometory::AddLine(v[2], v[6], color);
        Geometory::AddLine(v[3], v[7], color);
    }

    void DrawShadow(Texture* texture, const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& size)
    {
        if (!texture) return;

        const float kShadowScale = 1.25f;
        const float kShadowY = 0.001f;

        DirectX::XMMATRIX R = DirectX::XMMatrixRotationX(DirectX::XM_PIDIV2);
        DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(pos.x, kShadowY, pos.z);
        DirectX::XMFLOAT4X4 world;
        DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranspose(R * T));

        Sprite::SetWorld(world);
        Sprite::SetSize({ size.x * kShadowScale, size.z * kShadowScale });
        Sprite::SetOffset({ 0.0f, 0.0f });
        Sprite::SetUVPos({ 0.0f, 0.0f });
        Sprite::SetUVScale({ 1.0f, 1.0f });
        Sprite::SetColor({ 0.0f, 0.0f, 0.0f, 0.6f });
        Sprite::SetTexture(texture);
        Sprite::Draw();
    }

    void DrawAttackMarker(Texture* texture, const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& size)
    {
        if (!texture) return;

        const float kMarkerY = 0.002f;
        const float kMarkerScale = 1.0f;

        DirectX::XMMATRIX R = DirectX::XMMatrixRotationX(DirectX::XM_PIDIV2);
        DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(pos.x, kMarkerY, pos.z);
        DirectX::XMFLOAT4X4 world;
        DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranspose(R * T));

        Sprite::SetWorld(world);
        Sprite::SetSize({ size.x * kMarkerScale, size.z * kMarkerScale });
        Sprite::SetOffset({ 0.0f, 0.0f });
        Sprite::SetUVPos({ 0.0f, 0.0f });
        Sprite::SetUVScale({ 1.0f, 1.0f });
        Sprite::SetColor({ 1.0f, 1.0f, 1.0f, 0.9f });
        Sprite::SetTexture(texture);
        Sprite::Draw();
    }
}

SceneGame::SceneGame()
    : m_pCamera(nullptr)
    , m_pCameraGame(nullptr)
    , m_pCameraDebug(nullptr)
    , m_cameraMode(0)
    , m_pPlayer(nullptr)
    , m_pEnemy(nullptr)
    , m_enemyWasOverlapping(false)
    , m_pShadow(nullptr)
    , m_pAttackMarker(nullptr)
    , m_pEnemyHpFrame(nullptr)
    , m_pEnemyHpGauge(nullptr)
    , m_pHpFrame(nullptr)
    , m_pHpGauge(nullptr)
    , m_pGoal(nullptr)
    , m_stageSize(5.0f)
    , m_attackActive(false)
    , m_attackTimer(0.0f)
    , m_attackHitThisSwing(false)
    , m_lastMoveDir(0.0f, 0.0f, 1.0f)
    , m_attackCenter(0.0f, 0.0f, 0.0f)
    , m_attackSize(0.0f, 0.0f, 0.0f)
{
    m_pCameraGame = new CameraDebug();
    m_pCameraDebug = new CameraDebug();
    TRAN_INS;
    tran.cameraMode = NormalizeCameraMode(tran.cameraMode);
    m_cameraMode = tran.cameraMode;
    if (m_pCameraGame)
    {
        m_pCameraGame->LockPos(false);
        ApplyCameraPose(m_pCameraGame, tran.cameraGame.eye, tran.cameraGame.look);
    }
    if (m_pCameraDebug)
    {
        m_pCameraDebug->LockPos(false);
        ApplyCameraPose(m_pCameraDebug, tran.cameraDebug.eye, tran.cameraDebug.look);
    }
    m_pCamera = (m_cameraMode == kCameraModeDebug) ? static_cast<Camera*>(m_pCameraDebug)
        : static_cast<Camera*>(m_pCameraGame);
    tran.camera = (m_cameraMode == kCameraModeDebug) ? tran.cameraDebug : tran.cameraGame;

    m_pPlayer = new Player(m_pCamera);
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
        m_pEnemy->SetCamera(m_pCamera);
        m_pEnemy->SetSize(tran.player.size);
        m_pEnemy->SetPos({ 2.0f, 0.0f, 0.0f });
    }

    m_pShadow = new Texture();
    if (FAILED(m_pShadow->Create("Assets/Texture/Shadow.png")))
    {
        MessageBox(NULL, "Texture load failed.\nShadow.png", "Error", MB_OK);
    }

    m_pAttackMarker = new Texture();
    if (FAILED(m_pAttackMarker->Create("Assets/Texture/Star.png")))
    {
        MessageBox(NULL, "Texture load failed.\nStar.png", "Error", MB_OK);
    }

    m_pEnemyHpFrame = new Texture();
    if (FAILED(m_pEnemyHpFrame->Create("Assets/Texture/UIFrame.png")))
    {
        MessageBox(NULL, "Texture load failed.\nUIFrame.png", "Error", MB_OK);
    }

    m_pEnemyHpGauge = new Texture();
    if (FAILED(m_pEnemyHpGauge->Create("Assets/Texture/UIGauge.png")))
    {
        MessageBox(NULL, "Texture load failed.\nUIGauge.png", "Error", MB_OK);
    }

    //m_pGoal = new Goal({ 1.0f,1.0f,1.0f });
    //m_pGoal->SetCamera(m_pCamera);

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
    if (m_pShadow)
    {
        delete m_pShadow;
        m_pShadow = nullptr;
    }
    if (m_pAttackMarker)
    {
        delete m_pAttackMarker;
        m_pAttackMarker = nullptr;
    }
    if (m_pEnemyHpGauge)
    {
        delete m_pEnemyHpGauge;
        m_pEnemyHpGauge = nullptr;
    }
    if (m_pEnemyHpFrame)
    {
        delete m_pEnemyHpFrame;
        m_pEnemyHpFrame = nullptr;
    }
    if (m_pPlayer)
    {
        delete m_pPlayer;
        m_pPlayer = nullptr;
    }
    if (m_pCameraGame)
    {
        delete m_pCameraGame;
        m_pCameraGame = nullptr;
    }
    if (m_pCameraDebug)
    {
        delete m_pCameraDebug;
        m_pCameraDebug = nullptr;
    }
    m_pCamera = nullptr;
    if (m_pGoal)
    {
        delete m_pGoal;
        m_pGoal = nullptr;
    }
}

void SceneGame::Update()
{
    TRAN_INS;
    const int nextMode = NormalizeCameraMode(tran.cameraMode);
    if (nextMode != m_cameraMode)
    {
        m_cameraMode = nextMode;
        m_pCamera = (m_cameraMode == kCameraModeDebug) ? static_cast<Camera*>(m_pCameraDebug)
            : static_cast<Camera*>(m_pCameraGame);
        if (m_pPlayer) m_pPlayer->SetCamera(m_pCamera);
        if (m_pEnemy) m_pEnemy->SetCamera(m_pCamera);
        if (m_pGoal) m_pGoal->SetCamera(m_pCamera);
    }

    if (m_cameraMode == kCameraModeDebug)
    {
        tran.camera = tran.cameraDebug;
        ApplyCameraPose(m_pCameraDebug, tran.cameraDebug.eye, tran.cameraDebug.look);
    }
    else
    {
        tran.camera = tran.cameraGame;
        ApplyCameraPose(m_pCameraGame, tran.cameraGame.eye, tran.cameraGame.look);
    }

    if (m_pCamera) m_pCamera->Update();

    if (m_cameraMode == kCameraModeDebug)
    {
        tran.cameraDebug = tran.camera;
    }
    else
    {
        tran.cameraGame = tran.camera;
    }
if (m_pPlayer) m_pPlayer->Update();
    if (m_pEnemy) m_pEnemy->Update();
    if (m_pEnemy&&m_pPlayer)m_pEnemy->SetTargetPos(m_pPlayer->GetPos());
    if (m_pEnemy)
    {
        TRAN_INS;
        m_pEnemy->SetSize(tran.player.size);
        float stageSize = m_stageSize;
        if (tran.player.stageSize > 0.0f)
        {
            stageSize = tran.player.stageSize;
        }
        m_pEnemy->SetStageSize(stageSize);
    }

    {
        TRAN_INS;
        const float vLen = std::sqrt(tran.player.velocity.x * tran.player.velocity.x + tran.player.velocity.z * tran.player.velocity.z);
        if (vLen > 0.001f)
        {
            m_lastMoveDir = {
                tran.player.velocity.x / vLen,
                0.0f,
                tran.player.velocity.z / vLen
            };
        }
    }

    if (IsKeyTrigger('F'))
    {
        m_attackActive = true;
        m_attackTimer = kAttackDuration;
        m_attackHitThisSwing = false;
    }

    if (m_attackActive)
    {
        m_attackTimer -= kFixedDt;
        if (m_attackTimer <= 0.0f)
        {
            m_attackTimer = 0.0f;
            m_attackActive = false;
        }
    }

    if (m_attackActive)
    {
        TRAN_INS;
        m_attackSize = {
            tran.player.size.x,
            tran.player.size.y,
            tran.player.size.z * 1.5f
        };
        const float forward = (tran.player.size.z * 0.5f) + (m_attackSize.z * 0.5f);
        m_attackCenter = {
            tran.player.pos.x + m_lastMoveDir.x * forward,
            tran.player.pos.y + tran.player.size.y * 0.5f,
            tran.player.pos.z + m_lastMoveDir.z * forward
        };
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

        if (m_attackActive && !m_attackHitThisSwing)
        {
            Collision::Box attackBox{};
            attackBox.center = m_attackCenter;
            attackBox.size = m_attackSize;
            if (Collision::Hit(attackBox, enemyBox).isHit)
            {
                m_pEnemy->Damage(1);
                m_attackHitThisSwing = true;
            }
        }

        if (m_pEnemy && !m_pEnemy->IsAlive())
        {
            delete m_pEnemy;
            m_pEnemy = nullptr;
            m_enemyWasOverlapping = false;
        }
    }

    if(m_pGoal)
    {
        m_pGoal->Update();
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

    SetDepthTest(false);

    if (m_attackActive)
    {
        DrawAttackMarker(m_pAttackMarker, m_attackCenter, m_attackSize);
    }

    if (m_pCamera && m_pEnemy && m_pPlayer)
    {
        DirectX::XMFLOAT3 cam = m_pCamera->GetPos();
        DirectX::XMFLOAT3 playerPos;
        DirectX::XMFLOAT3 playerSize;
        DirectX::XMFLOAT3 enemyPos;
        DirectX::XMFLOAT3 enemySize;
        {
            TRAN_INS;
            playerPos = {
                tran.player.pos.x,
                tran.player.pos.y + tran.player.size.y * 0.5f,
                tran.player.pos.z
            };
            playerSize = tran.player.size;
        }
        {
            Collision::Box enemyBox = m_pEnemy->GetCollision();
            enemyPos = enemyBox.center;
            enemySize = m_pEnemy->GetSize();
        }

        const float dxP = playerPos.x - cam.x;
        const float dyP = playerPos.y - cam.y;
        const float dzP = playerPos.z - cam.z;
        const float dxE = enemyPos.x - cam.x;
        const float dyE = enemyPos.y - cam.y;
        const float dzE = enemyPos.z - cam.z;
        const float distP = dxP * dxP + dyP * dyP + dzP * dzP;
        const float distE = dxE * dxE + dyE * dyE + dzE * dzE;

        if (distE > distP)
        {
            DrawShadow(m_pShadow, enemyPos, enemySize);
            m_pEnemy->Draw();
            DrawShadow(m_pShadow, playerPos, playerSize);
            m_pPlayer->Draw();
        }
        else
        {
            DrawShadow(m_pShadow, playerPos, playerSize);
            m_pPlayer->Draw();
            DrawShadow(m_pShadow, enemyPos, enemySize);
            m_pEnemy->Draw();
        }
    }
    else
    {
        if (m_pEnemy)
        {
            DirectX::XMFLOAT3 enemyPos = m_pEnemy->GetCollision().center;
            DirectX::XMFLOAT3 enemySize = m_pEnemy->GetSize();
            DrawShadow(m_pShadow, enemyPos, enemySize);
            m_pEnemy->Draw();
        }
        if (m_pPlayer)
        {
            DirectX::XMFLOAT3 playerPos;
            DirectX::XMFLOAT3 playerSize;
            {
                TRAN_INS;
                playerPos = {
                    tran.player.pos.x,
                    tran.player.pos.y + tran.player.size.y * 0.5f,
                    tran.player.pos.z
                };
                playerSize = tran.player.size;
            }
            DrawShadow(m_pShadow, playerPos, playerSize);
            m_pPlayer->Draw();
        }
    }

    {
        TRAN_INS;
        Collision::Box playerBox{};
        playerBox.size = tran.player.size;
        playerBox.center = {
            tran.player.pos.x,
            tran.player.pos.y + tran.player.size.y * 0.5f,
            tran.player.pos.z
        };
        if (m_cameraMode == kCameraModeDebug)
        {
            if (m_pCameraGame)
            {
                AddCameraFrustumLines(*m_pCameraGame, { 1.0f, 1.0f, 0.0f, 1.0f });
            }
        }
        else if (m_pCamera)
        {
            AddCameraFrustumLines(*m_pCamera, { 1.0f, 1.0f, 0.0f, 1.0f });
        }

        AddAabbLines(playerBox, { 0.0f, 1.0f, 0.0f, 1.0f });

        if (m_pEnemy)
        {
            Collision::Box enemyBox = m_pEnemy->GetCollision();
            AddAabbLines(enemyBox, { 0.0f, 1.0f, 0.0f, 1.0f });
        }

        if (m_attackActive)
        {
            Collision::Box attackBox{};
            attackBox.center = m_attackCenter;
            attackBox.size = m_attackSize;
            AddAabbLines(attackBox, { 1.0f, 0.0f, 0.0f, 1.0f });
        }
    }

    Geometory::DrawLines();

    if (m_pEnemy && m_pEnemyHpFrame && m_pEnemyHpGauge && m_pCamera)
    {
        Collision::Box enemyBox = m_pEnemy->GetCollision();
        DirectX::XMFLOAT3 headPos = {
            enemyBox.center.x,
            enemyBox.center.y + enemyBox.size.y * kEnemyHpBillboardOffsetScale,
            enemyBox.center.z
        };

        float maxHp = static_cast<float>(m_pEnemy->GetMaxHp());
        float hp = static_cast<float>(m_pEnemy->GetHp());
        if (maxHp <= 0.0f) maxHp = 1.0f;
        const float rate = Clamp01(hp / maxHp);

        DrawEnemyHpGaugeBillboard(headPos, enemyBox.size, rate);
    }

    if(m_pGoal)
    {
        m_pGoal->Draw();
    }

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

void SceneGame::DrawEnemyHpGaugeBillboard(const DirectX::XMFLOAT3& headPos,
                                          const DirectX::XMFLOAT3& enemySize,
                                          float rate)
{
    if (!m_pEnemyHpGauge || !m_pEnemyHpFrame || !m_pCamera) return;

    const float clampedRate = Clamp01(rate);
    float width = enemySize.x * kEnemyHpBillboardWidthScale;
    float height = enemySize.y * kEnemyHpBillboardHeightScale;
    if (width < kEnemyHpBillboardMinWidth) width = kEnemyHpBillboardMinWidth;
    if (height < kEnemyHpBillboardMinHeight) height = kEnemyHpBillboardMinHeight;

    const float padding = height * kEnemyHpBillboardPaddingScale;
    const float gaugeWidth = width - padding * 2.0f;
    const float gaugeHeight = height - padding * 2.0f;

    DirectX::XMFLOAT3 camPos = m_pCamera->GetPos();
    DirectX::XMFLOAT3 camLook = m_pCamera->GetLook();
    DirectX::XMVECTOR camV = DirectX::XMLoadFloat3(&camPos);
    DirectX::XMVECTOR lookV = DirectX::XMLoadFloat3(&camLook);
    DirectX::XMVECTOR forward = DirectX::XMVectorSubtract(lookV, camV);
    if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(forward)) < 1.0e-6f)
    {
        forward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    }
    else
    {
        forward = DirectX::XMVector3Normalize(forward);
    }

    DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR right = DirectX::XMVector3Cross(up, forward);
    if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(right)) < 1.0e-6f)
    {
        up = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        right = DirectX::XMVector3Cross(up, forward);
    }
    right = DirectX::XMVector3Normalize(right);
    up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(forward, right));

    DirectX::XMFLOAT3 rightAxis;
    DirectX::XMStoreFloat3(&rightAxis, right);

    const float gaugeCenterShift = (clampedRate - 1.0f) * gaugeWidth * 0.5f;
    DirectX::XMFLOAT3 gaugePos = {
        headPos.x + rightAxis.x * gaugeCenterShift,
        headPos.y + rightAxis.y * gaugeCenterShift,
        headPos.z + rightAxis.z * gaugeCenterShift
    };

    DirectX::XMMATRIX R(right, up, forward, DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));

    DirectX::XMFLOAT4X4 world;
    DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranspose(R * DirectX::XMMatrixTranslation(headPos.x, headPos.y, headPos.z)));
    Sprite::SetWorld(world);
    Sprite::SetSize({ width, height });
    Sprite::SetOffset({ 0.0f, 0.0f });
    Sprite::SetUVPos({ 0.0f, 0.0f });
    Sprite::SetUVScale({ 1.0f, 1.0f });
    Sprite::SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    Sprite::SetTexture(m_pEnemyHpFrame);
    Sprite::Draw();

    if (clampedRate > 0.0f)
    {
        DirectX::XMFLOAT4X4 gaugeWorld;
        DirectX::XMStoreFloat4x4(&gaugeWorld, DirectX::XMMatrixTranspose(R * DirectX::XMMatrixTranslation(gaugePos.x, gaugePos.y, gaugePos.z)));
        Sprite::SetWorld(gaugeWorld);
        Sprite::SetSize({ gaugeWidth * clampedRate, gaugeHeight });
        Sprite::SetOffset({ 0.0f, 0.0f });
        Sprite::SetUVPos({ 0.0f, 0.0f });
        Sprite::SetUVScale({ clampedRate, 1.0f });
        Sprite::SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        Sprite::SetTexture(m_pEnemyHpGauge);
        Sprite::Draw();
    }
}







