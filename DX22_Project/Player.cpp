#include "Player.h"
#include "Input.h"
#include "Transfer.h"
#include <cmath>
#include "Defines.h"

namespace
{
    const char* kPlayerTexture = "Assets/Texture/Chracter/genbaneko.png";
    const float kDefaultStageSize = 5.0f;
    const float kDefaultMoveSpeed = 2.4f;
    const float kDefaultMaxHp = 100.0f;
    const float kDefaultDashDistance = 1.4f;
    const float kDefaultDashCooldown = 0.4f;
    const float kDefaultDashDuration = 0.12f;
    const float kMoveDt = 1.0f / 60.0f;

    float ClampFloat(float v, float lo, float hi)
    {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    float CalcEvadeCooldownScale(int evadeCooldownLevel)
    {
        const float scale = 1.0f - 0.10f * static_cast<float>(evadeCooldownLevel);
        return ClampFloat(scale, 0.30f, 1.0f);
    }
}


Player::Player(Camera*camera)
    : m_pTexture(nullptr)
    , m_size(0.5f, 1.0f, 0.5f)
    , m_velocity(0.0f, 0.0f, 0.0f)
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_hp(kDefaultMaxHp)
    , m_maxHp(kDefaultMaxHp)
    , m_moveSpeed(kDefaultMoveSpeed)
    , m_dashDistance(kDefaultDashDistance)
    , m_dashCooldown(kDefaultDashCooldown)
    , m_dashDuration(kDefaultDashDuration)
    , m_dashTimer(0.0f)
    , m_dashCooldownTimer(0.0f)
    , m_dashDir(0.0f, 0.0f, 1.0f)
    , m_isDashing(false)
    , m_stageSize(kDefaultStageSize)
    , m_pTrail(new TrailEffect(this))
    , m_pCamera(camera)
    , m_pTrailEffectTexture(nullptr)
{
    m_pTrail->AddLine(20);
    m_pos = { 0.0f, 0.0f, 0.0f };

    m_pTexture = new Texture();
    if (FAILED(m_pTexture->Create(kPlayerTexture)))
    {
        MessageBox(NULL, "Texture load failed.\nPlayer.cpp", "Error", MB_OK);
    }

    m_pTrailEffectTexture = new Texture();
    if (FAILED(m_pTrailEffectTexture->Create("Assets/Texture/Chracter/genbaneko.png")))
    {
        MessageBox(NULL, "Texture load failed.\nPlayer.cpp", "Error", MB_OK);
    }

    SyncToTransfer();
}

Player::~Player()
{
    if (m_pTrail)
    {
        delete m_pTrail;
        m_pTrail = nullptr;
    }
    if (m_pTexture)
    {
        delete m_pTexture;
        m_pTexture = nullptr;
    }
    if (m_pTrailEffectTexture)
    {
        delete m_pTrailEffectTexture;
        m_pTrailEffectTexture = nullptr;
    }
}

void Player::Update()
{
    SyncFromTransfer();
    ApplyMovement(kMoveDt);
    ClampToStage();

#ifdef _DEBUG
    if (IsKeyTrigger('P'))
        m_hp += 1.0f;

    if (m_hp < 0.0f)m_hp = 0;
#endif

    SyncToTransfer();
    m_pTrail->Update();
}

void Player::Draw()
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

    DirectX::XMMATRIX T = billboard *
        DirectX::XMMatrixTranslation(
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



    if (m_pTrail)
    {
        m_pTrail->SetView(m_pCamera->GetViewMatrix());
        m_pTrail->SetProjection(m_pCamera->GetProjectionMatrix());
        // テクスチャの設定(設定なし)
        //m_pTrail->SetTexture(m_pTrailEffectTexture);
        m_pTrail->Draw();
    }
}

void Player::SetCamera(Camera* set)
{
    m_pCamera = set;
}
void Player::SyncFromTransfer()
{
    TRAN_INS;
    m_pos = tran.player.pos;
    m_size = tran.player.size;
    m_velocity = tran.player.velocity;
    m_color = tran.player.color;
    m_hp = tran.player.hp;
    m_maxHp = tran.player.maxHp;
    m_moveSpeed = tran.player.moveSpeed;
    m_dashDistance = tran.player.dashDistance;
    m_dashCooldown = tran.player.dashCooldown;
    m_dashDuration = tran.player.dashDuration;
    m_stageSize = tran.player.stageSize;
}

void Player::SyncToTransfer()
{
    TRAN_INS;
    tran.player.pos = m_pos;
    tran.player.size = m_size;
    tran.player.velocity = m_velocity;
    tran.player.color = m_color;
    tran.player.hp = m_hp;
    tran.player.maxHp = m_maxHp;
    tran.player.moveSpeed = m_moveSpeed;
    tran.player.dashDistance = m_dashDistance;
    tran.player.dashCooldown = m_dashCooldown;
    tran.player.dashDuration = m_dashDuration;
    tran.player.stageSize = m_stageSize;
    tran.gameplayDebug.playerEvading = m_isDashing ? 1 : 0;
    tran.gameplayDebug.playerEvadeCooldownScale = CalcEvadeCooldownScale(tran.roguelike.evadeCooldownLevel);
}

void Player::ApplyMovement(float dt)
{
    TRAN_INS;
    float dirX = 0.0f;
    float dirZ = 0.0f;

    if (IsKeyPress('W')) dirZ += 1.0f;
    if (IsKeyPress('S')) dirZ -= 1.0f;
    if (IsKeyPress('A')) dirX -= 1.0f;
    if (IsKeyPress('D')) dirX += 1.0f;

    float len = sqrtf(dirX * dirX + dirZ * dirZ);
    if (len > 0.0001f)
    {
        dirX /= len;
        dirZ /= len;
    }
    else
    {
        dirX = 0.0f;
        dirZ = 0.0f;
    }

    const float evadeCooldownScale = CalcEvadeCooldownScale(tran.roguelike.evadeCooldownLevel);
    const float effectiveDashCooldown = m_dashCooldown * evadeCooldownScale;
    tran.gameplayDebug.playerEvadeCooldownScale = evadeCooldownScale;

    if (m_dashCooldownTimer > 0.0f)
    {
        m_dashCooldownTimer -= dt;
        if (m_dashCooldownTimer < 0.0f) m_dashCooldownTimer = 0.0f;
    }

    if (!m_isDashing && IsKeyTrigger(VK_SHIFT) && m_dashCooldownTimer <= 0.0f && len > 0.0001f)
    {
        if (m_dashDistance > 0.0f && m_dashDuration > 0.0f)
        {
            m_isDashing = true;
            m_dashTimer = m_dashDuration;
            m_dashCooldownTimer = effectiveDashCooldown;
            m_dashDir = DirectX::XMFLOAT3(dirX, 0.0f, dirZ);
        }
    }

    if (m_isDashing)
    {
        const float duration = m_dashDuration;
        const float distance = m_dashDistance;
        if (duration > 0.0f && distance > 0.0f)
        {
            const float speed = distance / duration;
            m_velocity.x = m_dashDir.x * speed;
            m_velocity.y = 0.0f;
            m_velocity.z = m_dashDir.z * speed;
            m_pos.x += m_velocity.x * dt;
            m_pos.z += m_velocity.z * dt;
        }

        m_dashTimer -= dt;
        if (m_dashTimer <= 0.0f)
        {
            m_dashTimer = 0.0f;
            m_isDashing = false;
        }
        return;
    }

    m_velocity.x = dirX * m_moveSpeed;
    m_velocity.y = 0.0f;
    m_velocity.z = dirZ * m_moveSpeed;
    m_pos.x += m_velocity.x * dt;
    m_pos.z += m_velocity.z * dt;
}

void Player::ClampToStage()
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

    m_pos.x = ClampFloat(m_pos.x, minX, maxX);
    m_pos.z = ClampFloat(m_pos.z, minZ, maxZ);
    m_pos.y = 0.0f;
}


