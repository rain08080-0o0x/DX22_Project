#include "SceneGame.h"
#include "Enemy.h"
#include "CameraDebug.h"
#include "Geometory.h"
#include "Sprite.h"
#include "DirectX.h"
#include "Input.h"
#include "Defines.h"
#include "Transfer.h"
#include "Main.h"
#include "SceneManager.h"
#include "UIObject.h"
#include "Collision.h"
#include "Texture.h"
#include "Sound.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace
{
    // UI layout and gameplay guard values used inside SceneGame only.
    const float kUiMargin = 20.0f;
    const float kHpFrameWidth = 260.0f;
    const float kHpFrameHeight = 32.0f;
    const float kHpGaugePadding = 4.0f;
    const float kCooldownFrameWidth = 220.0f;
    const float kCooldownFrameHeight = 24.0f;
    const float kCooldownGaugePadding = 3.0f;
    const float kCooldownRowSpacing = 8.0f;
    const int kMultiHitShakeThresholdMin = 1;
    const int kMultiHitShakeThresholdMax = 16;
    const float kMultiHitShakeDurationDefault = 0.18f;
    const float kBossStompImpactFxDuration = 0.18f;
    const float kBossStompImpactShadowScale = 1.75f;
    const float kFixedDt = 1.0f / 60.0f;
    const int kEnemyCountMin = 0;
    const int kEnemyCountMax = 16;
    const int kWaveCountMin = 1;
    const int kWaveCountMax = 32;
    const float kMinDuration = 0.01f;
    const float kCameraIntroDurationMin = 0.10f;
    const float kCameraIntroDurationMax = 8.0f;
    const float kCameraIntroFocusDistanceMin = 0.50f;
    const float kCameraIntroFocusDistanceMax = 12.0f;
    const float kCameraIntroExpStrength = 5.0f;
    const float kCameraIntroEyeMoveRatio = 0.42f;
    const float kCameraIntroEyeLift = 0.45f;
    const float kPi = 3.14159265f;
    const float kEnemyHpBillboardWidthScale = 2.0f;
    const float kEnemyHpBillboardHeightScale = 0.2f;
    const float kEnemyHpBillboardPaddingScale = 0.2f;
    const float kEnemyHpBillboardOffsetScale = 0.7f;
    const float kEnemyHpBillboardMinWidth = 0.6f;
    const float kEnemyHpBillboardMinHeight = 0.1f;
    const float kDebugRangeBoxHeight = 0.05f;
    const DirectX::XMFLOAT4 kDebugEnemyColorOutRange = { 0.0f, 1.0f, 0.0f, 1.0f };
    const DirectX::XMFLOAT4 kDebugEnemyColorInRangeCooling = { 0.0f, 0.8f, 1.0f, 1.0f };
    const DirectX::XMFLOAT4 kDebugEnemyColorReady = { 1.0f, 1.0f, 0.0f, 1.0f };
    const DirectX::XMFLOAT4 kDebugEnemyColorWindup = { 1.0f, 0.4f, 0.0f, 1.0f };
    const DirectX::XMFLOAT4 kDebugEnemyColorHit = { 1.0f, 0.0f, 0.0f, 1.0f };
    const DirectX::XMFLOAT4 kDebugRangeColorOut = { 0.2f, 0.2f, 0.7f, 1.0f };
    const DirectX::XMFLOAT4 kDebugRangeColorIn = { 0.0f, 0.8f, 1.0f, 1.0f };
    const DirectX::XMFLOAT4 kEnemyProjectileColor = { 0.25f, 0.95f, 1.0f, 0.90f };
    const DirectX::XMFLOAT3 kEnemySpawnPositions[] =
    {
        { 2.0f, 0.0f, 0.0f },
        {-2.0f, 0.0f, 1.5f },
        { 0.0f, 0.0f,-2.0f },
    };
    const int kEnemySpawnPresetCount = static_cast<int>(sizeof(kEnemySpawnPositions) / sizeof(kEnemySpawnPositions[0]));

    const int kCameraModeGame = 0;
    const int kCameraModeDebug = 1;
    const int kPauseMenuContinue = 0;
    const int kPauseMenuOption = 1;
    const int kPauseMenuToTitle = 2;
    const int kPauseOptionMaster = 0;
    const int kPauseOptionBgm = 1;
    const int kPauseOptionSe = 2;
    const int kPauseOptionDisplay = 3;
    const int kPauseOptionBack = 4;
    const int kPauseOptionCount = 5;
    const float kOptionVolumeStep = 0.05f;

    /**
     * @brief カメラモード値を有効な 2 値へ正規化します。
     * @param mode 補正対象のモード値です。
     * @return Debug 指定時のみ Debug、それ以外は Game です。
     */
    int NormalizeCameraMode(int mode)
    {
        return (mode == kCameraModeDebug) ? kCameraModeDebug : kCameraModeGame;
    }

    /**
     * @brief インデックスを 0 から count-1 の範囲に循環させます。
     * @param value 補正対象の値です。
     * @param count 要素数です。
     * @return 折り返し済みインデックスです。
     */
    int WrapIndex(int value, int count)
    {
        // 項目数が無い場合は安全側で 0 を返します。
        if (count <= 0) return 0;
        int wrapped = value % count;
        // 負方向に動かした時も先頭未満にならないように折り返します。
        if (wrapped < 0) wrapped += count;
        return wrapped;
    }

    /**
     * @brief ポーズ系 UI の決定入力が押されたかを返します。
     * @return Enter / F / Space のいずれかがトリガーなら true です。
     */
    bool IsPauseConfirmTriggered()
    {
        return IsKeyTrigger(VK_RETURN) || IsKeyTrigger('F') || IsKeyTrigger(VK_SPACE);
    }

    /**
     * @brief 音量値を UI で扱う範囲に丸めます。
     * @param value 補正対象音量です。
     * @return 0.0 から 2.0 に丸めた音量です。
     */
    float ClampVolume(float value)
    {
        if (value < 0.0f) return 0.0f;
        if (value > 2.0f) return 2.0f;
        return value;
    }

    /**
     * @brief 敵スポーン位置をインデックスから決定します。
     * @param index 敵番号です。
     * @param stageSize 現在のステージサイズです。
     * @return スポーン用ワールド座標です。
     */
    DirectX::XMFLOAT3 CalcEnemySpawnPos(int index, float stageSize)
    {
        // 先頭数体は決め打ち位置を使い、序盤の見え方を安定させます。
        if (index >= 0 && index < kEnemySpawnPresetCount)
        {
            return kEnemySpawnPositions[index];
        }

        // それ以外は円周上に均等配置し、ステージ外へ出にくい半径を使います。
        const float safeStage = (stageSize > 0.5f) ? stageSize : 5.0f;
        const float radius = safeStage * 0.35f;
        const float angle = (2.0f * kPi * static_cast<float>(index)) / static_cast<float>(kEnemyCountMax);
        return { std::cos(angle) * radius, 0.0f, std::sin(angle) * radius };
    }

    /**
     * @brief カメラが存在する場合だけ Eye / Look を反映します。
     * @param camera 更新するカメラです。
     * @param eye 新しい Eye です。
     * @param look 新しい Look です。
     */
    void ApplyCameraPose(CameraDebug* camera, const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& look)
    {
        if (camera)
        {
            camera->SetPose(eye, look);
        }
    }


    /**
     * @brief 浮動小数を 0.0 から 1.0 に丸めます。
     * @param v 補正対象です。
     * @return 0.0 から 1.0 に収めた値です。
     */
    float Clamp01(float v)
    {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    }

    /**
     * @brief 3 次元ベクトルを線形補間します。
     * @param a 開始値です。
     * @param b 終了値です。
     * @param t 補間係数です。
     * @return a から b の間を補間したベクトルです。
     */
    DirectX::XMFLOAT3 LerpFloat3(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, float t)
    {
        const float rate = Clamp01(t);
        return {
            a.x + (b.x - a.x) * rate,
            a.y + (b.y - a.y) * rate,
            a.z + (b.z - a.z) * rate
        };
    }

    /**
     * @brief 3 次元ベクトルを正規化します。
     * @param v 正規化対象です。
     * @param fallback 長さ 0 に近い場合に返す代替値です。
     * @return 正規化ベクトル、または fallback です。
     */
    DirectX::XMFLOAT3 Normalize3(const DirectX::XMFLOAT3& v, const DirectX::XMFLOAT3& fallback)
    {
        const float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
        if (lenSq <= 1.0e-6f)
        {
            // 長さが無い方向は正規化できないため、呼び出し側の既定方向へ逃がします。
            return fallback;
        }
        const float invLen = 1.0f / std::sqrt(lenSq);
        return { v.x * invLen, v.y * invLen, v.z * invLen };
    }

    /**
     * @brief カメラ導入演出に使う指数イージングを返します。
     * @param t 0.0 から 1.0 の進行率です。
     * @return イージング後の進行率です。
     */
    float ExpEase01(float t)
    {
        const float clamped = Clamp01(t);
        const float denom = 1.0f - expf(-kCameraIntroExpStrength);
        if (denom <= 1.0e-6f)
        {
            // 分母が壊れる場合は素直な線形値へフォールバックします。
            return clamped;
        }
        return (1.0f - expf(-kCameraIntroExpStrength * clamped)) / denom;
    }

    /**
     * @brief 浮動小数を任意範囲へ丸めます。
     * @param v 補正対象値です。
     * @param lo 下限です。
     * @param hi 上限です。
     * @return 指定範囲に収めた値です。
     */
    float ClampRange(float v, float lo, float hi)
    {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    /**
     * @brief 整数を任意範囲へ丸めます。
     * @param v 補正対象値です。
     * @param lo 下限です。
     * @param hi 上限です。
     * @return 指定範囲に収めた値です。
     */
    int ClampInt(int v, int lo, int hi)
    {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    /**
     * @brief 難易度による基準敵数補正を返します。
     * @param preset 難易度です。
     * @return Easy は -1、Hard は +1、Normal は 0 です。
     */
    int CalcDifficultyBaseEnemyBonus(int preset)
    {
        switch (preset)
        {
        case 0: return -1; // Easy
        case 2: return 1;  // Hard
        default: return 0; // Normal
        }
    }

    /**
     * @brief 難易度による Wave ごとの追加敵数補正を返します。
     * @param preset 難易度です。
     * @return Hard のみ +1、それ以外は 0 です。
     */
    int CalcDifficultyWaveAddBonus(int preset)
    {
        switch (preset)
        {
        case 0: return 0; // Easy
        case 2: return 1; // Hard
        default: return 0; // Normal
        }
    }

    /**
     * @brief 難易度による敵攻撃ダメージ倍率を返します。
     * @param preset 難易度です。
     * @return 難易度に応じたダメージ倍率です。
     */
    float CalcDifficultyEnemyAttackDamageScale(int preset)
    {
        switch (preset)
        {
        case 0: return 0.85f; // Easy
        case 2: return 1.25f; // Hard
        default: return 1.0f; // Normal
        }
    }

    /**
     * @brief XZ 平面上の距離二乗を返します。
     * @param a 座標 A です。
     * @param b 座標 B です。
     * @return XZ 平面距離の二乗です。
     */
    float DistSqXZ(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
    {
        const float dx = a.x - b.x;
        const float dz = a.z - b.z;
        return dx * dx + dz * dz;
    }

    /**
     * @brief XZ 平面成分だけを正規化します。
     * @param v 正規化対象ベクトルです。
     * @param fallback 長さ 0 に近い場合の代替値です。
     * @return XZ 正規化ベクトル、または fallback です。
     */
    DirectX::XMFLOAT3 NormalizeXZ(const DirectX::XMFLOAT3& v, const DirectX::XMFLOAT3& fallback)
    {
        const float len = std::sqrt(v.x * v.x + v.z * v.z);
        if (len <= 1.0e-6f)
        {
            return fallback;
        }
        return { v.x / len, 0.0f, v.z / len };
    }

    /**
     * @brief 中心とサイズから AABB を生成します。
     * @param center ボックス中心です。
     * @param size ボックスサイズです。
     * @return 組み立てた AABB です。
     */
    Collision::Box MakeAabb(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& size)
    {
        Collision::Box box{};
        box.center = center;
        box.size = size;
        return box;
    }

    /**
     * @brief AABB 同士のヒット判定を返します。
     * @param a 判定対象 A です。
     * @param b 判定対象 B です。
     * @return ヒットしていれば true です。
     */
    bool HitAabb(const Collision::Box& a, const Collision::Box& b)
    {
        // Gameplay collision policy: use AABB (Box vs Box) only.
        return Collision::Hit(a, b).isHit;
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

    /**
     * @brief 指定カメラのフラスタムを線で可視化します。
     * @param camera 可視化対象カメラです。
     * @param color 線色です。
     */
    void AddCameraFrustumLines(const Camera& camera, const DirectX::XMFLOAT4& color)
    {
        using namespace DirectX;

        const float nearZ = camera.GetNear();
        const float farZ = camera.GetFar();
        // near/far が不正だとフラスタムが組めないため描画しません。
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
            // Eye と Look が一致している場合は前方既定値を使います。
            forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        }
        else
        {
            forward = XMVector3Normalize(forward);
        }

        if (XMVectorGetX(XMVector3LengthSq(vUp)) < 1.0e-6f)
        {
            // Up が壊れている場合も描画を継続できるよう既定値に戻します。
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

    /**
     * @brief 足元に影スプライトを描画します。
     * @param texture 影テクスチャです。
     * @param pos 描画対象位置です。
     * @param size 対象サイズです。
     * @param scaleMultiplier 影サイズ倍率です。
     */
    void DrawShadow(Texture* texture,
                    const DirectX::XMFLOAT3& pos,
                    const DirectX::XMFLOAT3& size,
                    float scaleMultiplier = 1.0f)
    {
        // テクスチャ未設定時は描画できません。
        if (!texture) return;

        const float kShadowScale = 1.25f;
        const float kShadowY = 0.001f;

        DirectX::XMMATRIX R = DirectX::XMMatrixRotationX(DirectX::XM_PIDIV2);
        DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(pos.x, kShadowY, pos.z);
        DirectX::XMFLOAT4X4 world;
        DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranspose(R * T));

        Sprite::SetWorld(world);
        // 極端に小さい倍率でも影が消え切らないよう下限を設けます。
        const float finalScale = kShadowScale * ((scaleMultiplier > 0.01f) ? scaleMultiplier : 0.01f);
        Sprite::SetSize({ size.x * finalScale, size.z * finalScale });
        Sprite::SetOffset({ 0.0f, 0.0f });
        Sprite::SetUVPos({ 0.0f, 0.0f });
        Sprite::SetUVScale({ 1.0f, 1.0f });
        Sprite::SetColor({ 0.0f, 0.0f, 0.0f, 0.6f });
        Sprite::SetTexture(texture);
        Sprite::Draw();
    }

    /**
     * @brief 指定色付きの攻撃マーカーを地面へ描画します。
     * @param texture マーカー画像です。
     * @param pos 描画位置です。
     * @param size 描画サイズです。
     * @param color 表示色です。
     */
    void DrawAttackMarkerTint(Texture* texture,
                              const DirectX::XMFLOAT3& pos,
                              const DirectX::XMFLOAT3& size,
                              const DirectX::XMFLOAT4& color)
    {
        // テクスチャ未設定時は何も描画しません。
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
        Sprite::SetColor(color);
        Sprite::SetTexture(texture);
        Sprite::Draw();
    }

    /**
     * @brief 既定色の攻撃マーカーを描画します。
     * @param texture マーカー画像です。
     * @param pos 描画位置です。
     * @param size 描画サイズです。
     */
    void DrawAttackMarker(Texture* texture, const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& size)
    {
        DrawAttackMarkerTint(texture, pos, size, { 1.0f, 1.0f, 1.0f, 0.9f });
    }
}

/**
 * @brief ゲームシーンを生成し、戦闘に必要な各種オブジェクトを初期化します。
 */
SceneGame::SceneGame()
    : m_pCamera(nullptr)
    , m_pCameraGame(nullptr)
    , m_pCameraDebug(nullptr)
    , m_cameraMode(0)
    , m_pPlayer(nullptr)
    , m_enemies()
    , m_markerEffects()
    , m_enemyProjectiles()
    , m_boss()
    , m_pShadow(nullptr)
    , m_pAttackMarker(nullptr)
    , m_pBossAttackRangeMarker(nullptr)
    , m_pGroundTexture(nullptr)
    , m_pAttackSe(nullptr)
    , m_pPlayerHitSe(nullptr)
    , m_pEnemyAttackSe(nullptr)
    , m_pClearSe(nullptr)
    , m_pDropSe(nullptr)
    , m_pGameBgm(nullptr)
    , m_pBossBgm(nullptr)
    , m_pGameBgmVoice(nullptr)
    , m_isBossBgmActive(false)
    , m_pEnemyHpFrame(nullptr)
    , m_pEnemyHpGauge(nullptr)
    , m_pHpFrame(nullptr)
    , m_pHpGauge(nullptr)
    , m_pCooldownFrame{ nullptr, nullptr, nullptr, nullptr }
    , m_pCooldownGauge{ nullptr, nullptr, nullptr, nullptr }
    , m_stageSize(5.0f)
    , m_requestedEnemyCount(0)
    , m_currentWave(1)
    , m_waveMax(1)
    , m_cameraIntroActive(true)
    , m_cameraIntroTimer(0.0f)
    , m_cameraIntroStartEye(0.0f, 0.0f, 0.0f)
    , m_cameraIntroStartLook(0.0f, 0.0f, 0.0f)
    , m_cameraIntroFocusEye(0.0f, 0.0f, 0.0f)
    , m_cameraIntroFocusLook(0.0f, 0.0f, 0.0f)
    , m_attackActive(false)
    , m_attackTimer(0.0f)
    , m_attackWindupTimer(0.0f)
    , m_attackRecoveryTimer(0.0f)
    , m_attackCooldownTimer(0.0f)
    , m_attackCooldownUiTimer(0.0f)
    , m_attackCooldownUiDuration(0.0f)
    , m_skill1CooldownTimer(0.0f)
    , m_skill2CooldownTimer(0.0f)
    , m_skill1CooldownDuration(0.0f)
    , m_skill2CooldownDuration(0.0f)
    , m_attackSwingId(0)
    , m_attackHitCountThisSwing(0)
    , m_hitStopTimer(0.0f)
    , m_attackTrailSpawnTimer(0.0f)
    , m_playerDamageFlashTimer(0.0f)
    , m_playerDamageInvincibleTimer(0.0f)
    , m_bossStompImpactTimer(0.0f)
    , m_screenShakeTimer(0.0f)
    , m_screenShakeDuration(0.0f)
    , m_screenShakeAmplitude(0.0f)
    , m_screenShakePhase(0.0f)
    , m_enemyAttackSeGateTimer(0.0f)
    , m_enemyPerfPhase(0)
    , m_lastMoveDir(0.0f, 0.0f, 1.0f)
    , m_attackCenter(0.0f, 0.0f, 0.0f)
    , m_attackSize(0.0f, 0.0f, 0.0f)
    , m_isPaused(false)
    , m_pauseMenuSelection(kPauseMenuContinue)
    , m_isPauseOptionOpen(false)
    , m_pauseOptionSelection(kPauseOptionMaster)
    , m_isBossBattleDebug(false)
{
    // カメラはゲーム用とデバッグ用を分離して生成します。
    m_pCameraGame = new CameraDebug();
    m_pCameraDebug = new CameraDebug();
    TRAN_INS;

    // Transfer 側のモード値を正規化し、どちらのカメラを使うか決めます。
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

    // 現在アクティブなカメラ状態を共有領域へ反映します。
    tran.camera = (m_cameraMode == kCameraModeDebug) ? tran.cameraDebug : tran.cameraGame;

    // シーン開始時にリザルトやポーズ状態が残らないよう初期化します。
    SceneManager::ChangeResult(SceneManager::ResultType::None);
    tran.gameplayDebug.pauseMenuOpen = 0;
    tran.gameplayDebug.pauseMenuSelection = kPauseMenuContinue;
    tran.gameplayDebug.pauseMenuRequest = 0;
    tran.gameplayDebug.pauseOptionOpen = 0;
    tran.gameplayDebug.pauseOptionSelection = kPauseOptionMaster;
    tran.gameplayDebug.pauseOptionRequestClose = 0;
    tran.gameplayDebug.titleOptionOpen = 0;
    tran.gameplayDebug.titleOptionSelection = 0;
    tran.gameplayDebug.titleOptionRequestClose = 0;
    tran.gameplayDebug.titleDifficultyOpen = 0;
    tran.gameplayDebug.titleDifficultySelection = tran.NormalizeDifficultyPreset(tran.gameplayDebug.difficultyPreset);
    m_isBossBattleDebug = (tran.gameplayDebug.requestBossBattle != 0);
    tran.gameplayDebug.requestBossBattle = 0;
    tran.gameplayDebug.bossBattleActive = m_isBossBattleDebug ? 1 : 0;
    tran.gameplayDebug.showBossResultTimer = 0;
    tran.gameplayDebug.runTimerRunning = 1;

    m_pPlayer = new Player(m_pCamera);
    {
        TRAN_INS;
        if (tran.player.stageSize <= 0.0f)
        {
            tran.player.stageSize = m_stageSize;
        }
    }
    {
        TRAN_INS;
        const float introFocusDistance = ClampRange(
            tran.gameplay.cameraIntroFocusDistance,
            kCameraIntroFocusDistanceMin,
            kCameraIntroFocusDistanceMax);
        tran.gameplay.cameraIntroFocusDistance = introFocusDistance;
        const DirectX::XMFLOAT3 playerFocus = {
            tran.player.pos.x,
            tran.player.pos.y + tran.player.size.y * 0.55f,
            tran.player.pos.z
        };
        m_cameraIntroStartEye = tran.camera.eye;
        m_cameraIntroStartLook = tran.camera.look;
        m_cameraIntroFocusLook = playerFocus;

        const DirectX::XMFLOAT3 toPlayer = {
            playerFocus.x - m_cameraIntroStartEye.x,
            playerFocus.y - m_cameraIntroStartEye.y,
            playerFocus.z - m_cameraIntroStartEye.z
        };
        m_cameraIntroFocusEye = {
            m_cameraIntroStartEye.x + toPlayer.x * kCameraIntroEyeMoveRatio,
            m_cameraIntroStartEye.y + toPlayer.y * kCameraIntroEyeMoveRatio + kCameraIntroEyeLift,
            m_cameraIntroStartEye.z + toPlayer.z * kCameraIntroEyeMoveRatio
        };

        const DirectX::XMFLOAT3 fallbackBackDir = Normalize3({
            m_cameraIntroStartEye.x - m_cameraIntroStartLook.x,
            m_cameraIntroStartEye.y - m_cameraIntroStartLook.y,
            m_cameraIntroStartEye.z - m_cameraIntroStartLook.z
        }, { 0.0f, 0.2f, -1.0f });
        const DirectX::XMFLOAT3 focusBackDir = Normalize3({
            m_cameraIntroFocusEye.x - m_cameraIntroFocusLook.x,
            m_cameraIntroFocusEye.y - m_cameraIntroFocusLook.y,
            m_cameraIntroFocusEye.z - m_cameraIntroFocusLook.z
        }, fallbackBackDir);
        m_cameraIntroFocusEye = {
            m_cameraIntroFocusLook.x + focusBackDir.x * introFocusDistance,
            m_cameraIntroFocusLook.y + focusBackDir.y * introFocusDistance + kCameraIntroEyeLift,
            m_cameraIntroFocusLook.z + focusBackDir.z * introFocusDistance
        };
        m_cameraIntroActive = true;
        m_cameraIntroTimer = 0.0f;
    }

    m_enemies.clear();
    const int baseEnemyCount = static_cast<int>(ClampRange(
        static_cast<float>(tran.gameplay.enemyCount),
        static_cast<float>(kEnemyCountMin),
        static_cast<float>(kEnemyCountMax)));
    const int waveMax = static_cast<int>(ClampRange(
        static_cast<float>(tran.gameplay.waveMax),
        static_cast<float>(kWaveCountMin),
        static_cast<float>(kWaveCountMax)));
    const int waveEnemyAddPerWave = static_cast<int>(ClampRange(
        static_cast<float>(tran.gameplay.waveEnemyAddPerWave),
        0.0f,
        static_cast<float>(kEnemyCountMax)));
    const int difficultyPreset = ClampInt(tran.gameplayDebug.difficultyPreset, 0, 2);
    const int effectiveBaseEnemyCount = ClampInt(
        baseEnemyCount + CalcDifficultyBaseEnemyBonus(difficultyPreset),
        kEnemyCountMin,
        kEnemyCountMax);
    const int effectiveWaveEnemyAdd = ClampInt(
        waveEnemyAddPerWave + CalcDifficultyWaveAddBonus(difficultyPreset),
        0,
        kEnemyCountMax);

    m_currentWave = 1;
    m_waveMax = waveMax;
    m_requestedEnemyCount = CalcWaveEnemyCount(effectiveBaseEnemyCount, m_currentWave, effectiveWaveEnemyAdd);
    if (m_isBossBattleDebug)
    {
        m_currentWave = m_waveMax;
        m_requestedEnemyCount = 0;
    }

    tran.gameplay.enemyCount = baseEnemyCount;
    tran.gameplay.waveMax = m_waveMax;
    tran.gameplay.waveEnemyAddPerWave = waveEnemyAddPerWave;
    tran.roguelike.attackPowerLevel = tran.ClampUpgradeLevel(tran.roguelike.attackPowerLevel);
    tran.roguelike.attackSpeedLevel = tran.ClampUpgradeLevel(tran.roguelike.attackSpeedLevel);
    tran.roguelike.evadeCooldownLevel = tran.ClampUpgradeLevel(tran.roguelike.evadeCooldownLevel);
    tran.gameplayDebug.difficultyPreset = difficultyPreset;
    tran.gameplayDebug.effectiveEnemyBaseCount = effectiveBaseEnemyCount;
    tran.gameplayDebug.effectiveEnemyAddPerWave = effectiveWaveEnemyAdd;
    tran.gameplayDebug.stageClearCount = tran.roguelike.stageClearCount;
    tran.gameplayDebug.attackPowerLevel = tran.roguelike.attackPowerLevel;
    tran.gameplayDebug.attackSpeedLevel = tran.roguelike.attackSpeedLevel;
    tran.gameplayDebug.evadeCooldownLevel = tran.roguelike.evadeCooldownLevel;
    tran.gameplayDebug.lastUpgradeType = tran.roguelike.lastUpgradeType;
    tran.gameplayDebug.upgradeSelectionPending = tran.roguelike.selectionPending;
    tran.gameplayDebug.upgradeRerollRemain = tran.roguelike.rerollRemain;
    tran.gameplayDebug.upgradeOffer0 = tran.roguelike.offers[0];
    tran.gameplayDebug.upgradeOffer1 = tran.roguelike.offers[1];
    tran.gameplayDebug.upgradeOffer2 = tran.roguelike.offers[2];
    EnsureEnemyCount(m_requestedEnemyCount, tran.player.stageSize);
    InitializeBossForScene();

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
    m_pBossAttackRangeMarker = new Texture();
    if (FAILED(m_pBossAttackRangeMarker->Create("Assets/Texture/Game/AttackRange.png")))
    {
        MessageBox(NULL, "Texture load failed.\nGame/AttackRange.png", "Error", MB_OK);
    }
    m_pGroundTexture = new Texture();
    if (FAILED(m_pGroundTexture->Create("Assets/Texture/Game/jimen.png")))
    {
        MessageBox(NULL, "Texture load failed.\nGame/jimen.png", "Error", MB_OK);
        delete m_pGroundTexture;
        m_pGroundTexture = nullptr;
    }

    LoadBossResources();

    m_pAttackSe = LoadSound("Assets/Sound/SE/attack.mp3", false);
    m_pPlayerHitSe = LoadSound("Assets/Sound/SE/player_hit.mp3", false);
    m_pEnemyAttackSe = LoadSound("Assets/Sound/SE/enemy_attack.mp3", false);
    m_pClearSe = LoadSound("Assets/Sound/SE/clear.mp3", false);
    m_pDropSe = LoadSound("Assets/Sound/SE/drop.mp3", false);
    m_pGameBgm = LoadSound("Assets/Sound/BGM/GameBGM.mp3", true);
    m_pBossBgm = LoadSound("Assets/Sound/BGM/GameBGM2.mp3", true);
    if (m_pGameBgm)
    {
        m_pGameBgmVoice = PlaySound(m_pGameBgm);
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

    const DirectX::XMFLOAT4 cooldownColors[CooldownSlotCount] =
    {
        { 1.0f, 0.35f, 0.35f, 1.0f }, // Attack
        { 0.35f, 0.95f, 0.55f, 1.0f }, // Evade
        { 0.35f, 0.70f, 1.0f, 1.0f }, // Skill1
        { 1.0f, 0.80f, 0.35f, 1.0f }  // Skill2
    };
    for (int i = 0; i < CooldownSlotCount; ++i)
    {
        m_pCooldownFrame[i] = new UIObject("UIFrame.png", 0.0f, 0.0f, kCooldownFrameWidth, kCooldownFrameHeight);
        m_pCooldownGauge[i] = new UIObject("UIGauge.png", 0.0f, 0.0f,
                                           kCooldownFrameWidth - kCooldownGaugePadding * 2.0f,
                                           kCooldownFrameHeight - kCooldownGaugePadding * 2.0f);
        if (m_pCooldownGauge[i])
        {
            m_pCooldownGauge[i]->SetColor(cooldownColors[i]);
        }
        m_uiManager.Add(m_pCooldownGauge[i], UIObjectManager::Layer::Game);
        m_uiManager.Add(m_pCooldownFrame[i], UIObjectManager::Layer::Game);
    }

    UpdateHpGauge();
    UpdateCooldownGauges();
    tran.gameplayDebug.currentWave = m_currentWave;
    tran.gameplayDebug.maxWave = m_waveMax;
    tran.gameplayDebug.enemiesAlive = static_cast<int>(m_enemies.size());
    tran.gameplayDebug.enemiesTarget = m_requestedEnemyCount;
}

/**
 * @brief SceneGame が生成したリソースとオブジェクトを解放します。
 */
SceneGame::~SceneGame()
{
    // BGM 再生中なら先に停止し、ボイスを破棄します。
    if (m_pGameBgmVoice)
    {
        m_pGameBgmVoice->Stop();
        m_pGameBgmVoice->DestroyVoice();
        m_pGameBgmVoice = nullptr;
    }
    m_pClearSe = nullptr;
    m_pDropSe = nullptr;
    m_pEnemyAttackSe = nullptr;
    m_pPlayerHitSe = nullptr;
    m_pAttackSe = nullptr;
    m_pGameBgm = nullptr;
    m_pBossBgm = nullptr;
    m_isBossBgmActive = false;

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
    for (int i = 0; i < CooldownSlotCount; ++i)
    {
        if (m_pCooldownGauge[i])
        {
            delete m_pCooldownGauge[i];
            m_pCooldownGauge[i] = nullptr;
        }
        if (m_pCooldownFrame[i])
        {
            delete m_pCooldownFrame[i];
            m_pCooldownFrame[i] = nullptr;
        }
    }
    for (auto& slot : m_enemies)
    {
        if (slot.enemy)
        {
            delete slot.enemy;
            slot.enemy = nullptr;
        }
    }
    m_enemies.clear();
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
    if (m_pBossAttackRangeMarker)
    {
        delete m_pBossAttackRangeMarker;
        m_pBossAttackRangeMarker = nullptr;
    }
    if (m_pGroundTexture)
    {
        delete m_pGroundTexture;
        m_pGroundTexture = nullptr;
    }
    ReleaseBossResources();
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
}

/**
 * @brief 指定スロット向けに敵を 1 体生成して初期化します。
 * @param index 生成対象インデックスです。
 * @param stageSize スポーン計算に使うステージサイズです。
 */
void SceneGame::SpawnEnemyByIndex(int index, float stageSize)
{
    Enemy* enemy = new Enemy();
    // 生成失敗時は初期化を継続できないため中断します。
    if (!enemy) return;

    TRAN_INS;
    // 生成直後に共通設定をまとめて反映します。
    enemy->SetCamera(m_pCamera);
    enemy->SetSize(tran.player.size);
    enemy->SetStageSize(stageSize);
    // 生成順で敵タイプをローテーションし、編成を分散させます。
    switch ((index % 3 + 3) % 3)
    {
    case 0: enemy->SetType(Enemy::Type::Speed); break;
    case 1: enemy->SetType(Enemy::Type::Tank); break;
    default: enemy->SetType(Enemy::Type::Ranged); break;
    }
    const float enemyHpScale = m_isBossBattleDebug ? 1.0f : tran.GetEnemyHpScaleByUpgradeProgress();
    enemy->SetHpScale(enemyHpScale);

    // スポーン候補探索で使う値は先に丸めて、異常値でも破綻しないようにします。
    const float safeStage = (stageSize > 0.5f) ? stageSize : 5.0f;
    const float ringScale = ClampRange(tran.gameplay.enemySpawnRingScale, 0.1f, 0.9f);
    const float jitterScale = ClampRange(tran.gameplay.enemySpawnJitterScale, 0.0f, 0.5f);
    const float minPlayerDist = (tran.gameplay.enemySpawnMinPlayerDist < 0.0f) ? 0.0f : tran.gameplay.enemySpawnMinPlayerDist;
    const float minEnemyDist = (tran.gameplay.enemySpawnMinEnemyDist < 0.0f) ? 0.0f : tran.gameplay.enemySpawnMinEnemyDist;
    const float minPlayerDistSq = minPlayerDist * minPlayerDist;
    const float minEnemyDistSq = minEnemyDist * minEnemyDist;
    const float stageHalf = safeStage * 0.5f;

    auto clampToStage = [&](DirectX::XMFLOAT3 pos)
    {
        const float halfX = tran.player.size.x * 0.5f;
        const float halfZ = tran.player.size.z * 0.5f;
        float minX = -stageHalf + halfX;
        float maxX = stageHalf - halfX;
        float minZ = -stageHalf + halfZ;
        float maxZ = stageHalf - halfZ;
        if (minX > maxX) { minX = 0.0f; maxX = 0.0f; }
        if (minZ > maxZ) { minZ = 0.0f; maxZ = 0.0f; }
        pos.x = ClampRange(pos.x, minX, maxX);
        pos.z = ClampRange(pos.z, minZ, maxZ);
        pos.y = 0.0f;
        return pos;
    };

    const DirectX::XMFLOAT3 playerPos = m_pPlayer ? m_pPlayer->GetPos() : DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 bestPos = clampToStage(CalcEnemySpawnPos(index, safeStage));
    float bestScore = -1.0f;
    const float baseRadius = safeStage * ringScale;
    const float jitterRadius = safeStage * jitterScale;
    const int kSpawnAttempts = 24;

    // 複数候補から、プレイヤーと既存敵に対して最も余裕がある位置を選びます。
    for (int attempt = 0; attempt < kSpawnAttempts; ++attempt)
    {
        DirectX::XMFLOAT3 candidate{};
        if (attempt == 0)
        {
            candidate = CalcEnemySpawnPos(index, safeStage);
        }
        else
        {
            const float angle = (2.0f * kPi * (static_cast<float>(index) + static_cast<float>(attempt) * 0.6180339f))
                / static_cast<float>((kEnemyCountMax > 0) ? kEnemyCountMax : 1);
            const float jitterStep = static_cast<float>((attempt % 5) - 2) * 0.5f;
            float radius = baseRadius + jitterRadius * jitterStep;
            radius = ClampRange(radius, safeStage * 0.10f, safeStage * 0.48f);
            candidate = { std::cos(angle) * radius, 0.0f, std::sin(angle) * radius };
        }
        candidate = clampToStage(candidate);

        float nearestDistSq = DistSqXZ(candidate, playerPos);
        bool farFromPlayer = nearestDistSq >= minPlayerDistSq;
        bool farFromEnemies = true;
        for (const auto& slot : m_enemies)
        {
            if (!slot.enemy) continue;
            const float d = DistSqXZ(candidate, slot.enemy->GetPos());
            if (d < nearestDistSq) nearestDistSq = d;
            if (d < minEnemyDistSq)
            {
                farFromEnemies = false;
                break;
            }
        }

        if (farFromPlayer && farFromEnemies)
        {
            bestPos = candidate;
            break;
        }
        if (nearestDistSq > bestScore)
        {
            bestScore = nearestDistSq;
            bestPos = candidate;
        }
    }

    enemy->SetTargetPos(playerPos);
    enemy->SetPos(bestPos);

    EnemySlot slot{};
    slot.enemy = enemy;
    m_enemies.push_back(slot);
}

/**
 * @brief 目標数へ合わせて敵スロットを増減させます。
 * @param targetCount 目標敵数です。
 * @param stageSize スポーン計算に使うステージサイズです。
 */
void SceneGame::EnsureEnemyCount(int targetCount, float stageSize)
{
    const int clampedTarget = static_cast<int>(ClampRange(static_cast<float>(targetCount), static_cast<float>(kEnemyCountMin), static_cast<float>(kEnemyCountMax)));

    // 足りない間は必要数に達するまで生成を繰り返します。
    while (static_cast<int>(m_enemies.size()) < clampedTarget)
    {
        SpawnEnemyByIndex(static_cast<int>(m_enemies.size()), stageSize);
    }

    // 多い間は末尾から破棄して要求数へ揃えます。
    while (static_cast<int>(m_enemies.size()) > clampedTarget)
    {
        EnemySlot& back = m_enemies.back();
        if (back.enemy)
        {
            delete back.enemy;
            back.enemy = nullptr;
        }
        m_enemies.pop_back();
    }
}

/**
 * @brief Wave 番号から出現敵数を計算します。
 * @param baseCount 基本敵数です。
 * @param waveIndex 現在 Wave 番号です。
 * @param addPerWave Wave ごとの増加数です。
 * @return 計算後の敵数です。
 */
int SceneGame::CalcWaveEnemyCount(int baseCount, int waveIndex, int addPerWave) const
{
    // 基本値と Wave 値を安全範囲に補正し、負方向の増加は許可しません。
    int safeBase = baseCount;
    if (safeBase < kEnemyCountMin) safeBase = kEnemyCountMin;
    if (safeBase > kEnemyCountMax) safeBase = kEnemyCountMax;

    int safeWave = waveIndex;
    if (safeWave < kWaveCountMin) safeWave = kWaveCountMin;

    int safeAdd = addPerWave;
    if (safeAdd < 0) safeAdd = 0;

    // Wave 1 を基点に、追加数を累積して最終敵数を決めます。
    const int waveStep = safeWave - 1;
    const int expanded = safeBase + waveStep * safeAdd;
    if (expanded < kEnemyCountMin) return kEnemyCountMin;
    if (expanded > kEnemyCountMax) return kEnemyCountMax;
    return expanded;
}

/**
 * @brief 戦闘、UI、カメラ、ポーズ状態を 1 フレーム分更新します。
 */
void SceneGame::Update()
{
    TRAN_INS;
    // ポーズ UI の拡大率は許容範囲に丸め、異常値で表示が崩れないようにします。
    tran.gameplayDebug.pauseMenuUiScale = ClampRange(tran.gameplayDebug.pauseMenuUiScale, 0.5f, 2.5f);
    tran.gameplayDebug.pauseMenuFontScale = ClampRange(tran.gameplayDebug.pauseMenuFontScale, 0.5f, 2.5f);
    tran.gameplayDebug.pauseMenuButtonScale = ClampRange(tran.gameplayDebug.pauseMenuButtonScale, 0.5f, 2.5f);
    if (m_pGameBgmVoice)
    {
        // ポーズ中の Option 変更も即時反映するため、毎フレーム音量を適用します。
        const float masterVolume = ClampRange(tran.gameplay.volumeMaster, 0.0f, 2.0f);
        const float bgmVolume = ClampRange(tran.gameplay.volumeBgm, 0.0f, 2.0f);
        m_pGameBgmVoice->SetVolume(masterVolume * bgmVolume);
    }
    if (tran.gameplayDebug.pauseOptionRequestClose != 0)
    {
        tran.gameplayDebug.pauseOptionRequestClose = 0;
        m_isPauseOptionOpen = false;
    }

    if (IsKeyTrigger(VK_ESCAPE))
    {
        if (!m_isPaused)
        {
            m_isPaused = true;
            m_isPauseOptionOpen = false;
            m_pauseMenuSelection = kPauseMenuContinue;
        }
        else if (m_isPauseOptionOpen)
        {
            m_isPauseOptionOpen = false;
        }
        else
        {
            m_isPaused = false;
        }
        tran.gameplayDebug.pauseMenuRequest = 0;
    }

    if (m_isPaused)
    {
        // Pause state splits into menu navigation and option sub-menu navigation.
        if (m_isPauseOptionOpen)
        {
            if (IsKeyTrigger(VK_UP) || IsKeyTrigger('W'))
            {
                m_pauseOptionSelection = WrapIndex(m_pauseOptionSelection - 1, kPauseOptionCount);
            }
            if (IsKeyTrigger(VK_DOWN) || IsKeyTrigger('S'))
            {
                m_pauseOptionSelection = WrapIndex(m_pauseOptionSelection + 1, kPauseOptionCount);
            }

            const bool decrease = IsKeyTrigger(VK_LEFT) || IsKeyTrigger('A');
            const bool increase = IsKeyTrigger(VK_RIGHT) || IsKeyTrigger('D');
            if (decrease || increase)
            {
                const float delta = decrease ? -kOptionVolumeStep : kOptionVolumeStep;
                switch (m_pauseOptionSelection)
                {
                case kPauseOptionMaster:
                    tran.gameplay.volumeMaster = ClampVolume(tran.gameplay.volumeMaster + delta);
                    break;
                case kPauseOptionBgm:
                    tran.gameplay.volumeBgm = ClampVolume(tran.gameplay.volumeBgm + delta);
                    break;
                case kPauseOptionSe:
                    tran.gameplay.volumeSe = ClampVolume(tran.gameplay.volumeSe + delta);
                    break;
                case kPauseOptionDisplay:
                    SetAppFullscreen(increase);
                    break;
                default:
                    break;
                }
            }

            if (IsPauseConfirmTriggered())
            {
                if (m_pauseOptionSelection == kPauseOptionDisplay)
                {
                    ToggleAppFullscreen();
                }
                else if (m_pauseOptionSelection == kPauseOptionBack)
                {
                    m_isPauseOptionOpen = false;
                }
            }
        }
        else
        {
            if (IsKeyTrigger(VK_UP) || IsKeyTrigger('W') || IsKeyTrigger(VK_LEFT) || IsKeyTrigger('A'))
            {
                m_pauseMenuSelection = WrapIndex(m_pauseMenuSelection - 1, 3);
            }
            if (IsKeyTrigger(VK_DOWN) || IsKeyTrigger('S') || IsKeyTrigger(VK_RIGHT) || IsKeyTrigger('D'))
            {
                m_pauseMenuSelection = WrapIndex(m_pauseMenuSelection + 1, 3);
            }

            int actionRequest = tran.gameplayDebug.pauseMenuRequest;
            if (actionRequest < 0 || actionRequest > 3) actionRequest = 0;
            const bool confirmByKey = IsPauseConfirmTriggered();
            if (confirmByKey || actionRequest != 0)
            {
                const int action = (actionRequest != 0)
                    ? actionRequest
                    : ((m_pauseMenuSelection == kPauseMenuContinue) ? 1
                        : (m_pauseMenuSelection == kPauseMenuOption ? 3 : 2));
                tran.gameplayDebug.pauseMenuRequest = 0;

                if (action == 1)
                {
                    // Continue closes pause and resumes gameplay immediately.
                    m_isPaused = false;
                    m_isPauseOptionOpen = false;
                }
                else if (action == 2)
                {
                    // Returning to title also resets transient run/boss state.
                    m_isPaused = false;
                    m_isPauseOptionOpen = false;
                    tran.gameplayDebug.pauseMenuOpen = 0;
                    tran.gameplayDebug.pauseMenuSelection = kPauseMenuContinue;
                    tran.gameplayDebug.pauseOptionOpen = 0;
                    tran.gameplayDebug.pauseOptionSelection = kPauseOptionMaster;
                    tran.gameplayDebug.pauseOptionRequestClose = 0;
                    tran.gameplayDebug.runTimerRunning = 0;
                    tran.gameplayDebug.runRecordedSec = tran.gameplayDebug.runElapsedSec;
                    tran.gameplayDebug.requestBossBattle = 0;
                    tran.gameplayDebug.bossBattleActive = 0;
                    tran.gameplayDebug.showBossResultTimer = 0;
                    tran.ResetRoguelikeUpgrade();
                    SceneManager::ChangeResult(SceneManager::ResultType::None);
                    SceneManager::ChangeScene(SceneManager::SCENE_TITLE);
                    return;
                }
                else if (action == 3)
                {
                    // Option opens the nested pause option menu.
                    m_isPauseOptionOpen = true;
                    m_pauseOptionSelection = kPauseOptionMaster;
                    tran.gameplayDebug.pauseOptionRequestClose = 0;
                }
            }
        }
    }

    tran.gameplayDebug.pauseMenuOpen = m_isPaused ? 1 : 0;
    tran.gameplayDebug.pauseMenuSelection = m_pauseMenuSelection;
    tran.gameplayDebug.pauseOptionOpen = m_isPauseOptionOpen ? 1 : 0;
    tran.gameplayDebug.pauseOptionSelection = m_pauseOptionSelection;
    if (m_isPaused)
    {
        // Gameplay update stops completely while paused.
        return;
    }

    tran.gameplayDebug.pauseMenuRequest = 0;
    tran.gameplayDebug.pauseOptionOpen = 0;
    tran.gameplayDebug.pauseOptionSelection = kPauseOptionMaster;
    tran.gameplayDebug.pauseOptionRequestClose = 0;
    tran.gameplayDebug.bossBattleActive = m_isBossBattleDebug ? 1 : 0;
    int baseEnemyCount = tran.gameplay.enemyCount;
    if (baseEnemyCount < kEnemyCountMin) baseEnemyCount = kEnemyCountMin;
    if (baseEnemyCount > kEnemyCountMax) baseEnemyCount = kEnemyCountMax;
    tran.gameplay.enemyCount = baseEnemyCount;

    int waveMax = tran.gameplay.waveMax;
    if (waveMax < kWaveCountMin) waveMax = kWaveCountMin;
    if (waveMax > kWaveCountMax) waveMax = kWaveCountMax;
    tran.gameplay.waveMax = waveMax;
    m_waveMax = waveMax;

    int waveEnemyAddPerWave = tran.gameplay.waveEnemyAddPerWave;
    if (waveEnemyAddPerWave < 0) waveEnemyAddPerWave = 0;
    if (waveEnemyAddPerWave > kEnemyCountMax) waveEnemyAddPerWave = kEnemyCountMax;
    tran.gameplay.waveEnemyAddPerWave = waveEnemyAddPerWave;
    const int difficultyPreset = ClampInt(tran.gameplayDebug.difficultyPreset, 0, 2);
    tran.gameplayDebug.difficultyPreset = difficultyPreset;
    const int effectiveBaseEnemyCount = ClampInt(
        baseEnemyCount + CalcDifficultyBaseEnemyBonus(difficultyPreset),
        kEnemyCountMin,
        kEnemyCountMax);
    const int effectiveWaveEnemyAdd = ClampInt(
        waveEnemyAddPerWave + CalcDifficultyWaveAddBonus(difficultyPreset),
        0,
        kEnemyCountMax);
    const int playerAttackDamage = tran.GetPlayerAttackDamageByLevel(tran.roguelike.attackPowerLevel);
    const float playerAttackCooldownScale = tran.GetAttackCooldownScaleByLevel(tran.roguelike.attackSpeedLevel);
    const float playerEvadeCooldownScale = tran.GetEvadeCooldownScaleByLevel(tran.roguelike.evadeCooldownLevel);
    const float difficultyEnemyAttackDamageScale = CalcDifficultyEnemyAttackDamageScale(difficultyPreset);
    const float roguelikeEnemyAttackScale = m_isBossBattleDebug ? 1.0f : tran.GetEnemyAttackScaleByUpgradeProgress();

    if (m_currentWave < kWaveCountMin) m_currentWave = kWaveCountMin;
    if (m_currentWave > m_waveMax) m_currentWave = m_waveMax;

    const int waveEnemyTarget = CalcWaveEnemyCount(effectiveBaseEnemyCount, m_currentWave, effectiveWaveEnemyAdd);

    const float cameraIntroDuration = ClampRange(
        tran.gameplay.cameraIntroDuration,
        kCameraIntroDurationMin,
        kCameraIntroDurationMax);
    tran.gameplay.cameraIntroDuration = cameraIntroDuration;
    const float attackWindup = (tran.gameplay.attackWindup < 0.0f) ? 0.0f : tran.gameplay.attackWindup;
    const float attackDuration = (tran.gameplay.attackDuration < kMinDuration) ? kMinDuration : tran.gameplay.attackDuration;
    const float attackRecovery = (tran.gameplay.attackRecovery < 0.0f) ? 0.0f : tran.gameplay.attackRecovery;
    const float attackCooldown = ((tran.gameplay.attackCooldown < 0.0f) ? 0.0f : tran.gameplay.attackCooldown) * playerAttackCooldownScale;
    const float skill1Cooldown = (tran.gameplay.skill1Cooldown < 0.0f) ? 0.0f : tran.gameplay.skill1Cooldown;
    const float skill2Cooldown = (tran.gameplay.skill2Cooldown < 0.0f) ? 0.0f : tran.gameplay.skill2Cooldown;
    int multiHitShakeThreshold = tran.gameplay.screenShakeHitThreshold;
    if (multiHitShakeThreshold < kMultiHitShakeThresholdMin) multiHitShakeThreshold = kMultiHitShakeThresholdMin;
    if (multiHitShakeThreshold > kMultiHitShakeThresholdMax) multiHitShakeThreshold = kMultiHitShakeThresholdMax;
    tran.gameplay.screenShakeHitThreshold = multiHitShakeThreshold;
    const float multiHitShakeDuration = (tran.gameplay.screenShakeDuration < 0.0f)
        ? 0.0f : tran.gameplay.screenShakeDuration;
    const float multiHitShakeAmplitude = (tran.gameplay.screenShakeAmplitude < 0.0f)
        ? 0.0f : tran.gameplay.screenShakeAmplitude;
    const float attackSweepDegrees = tran.gameplay.attackSweepDegrees;
    const float attackSweepRadiusScale = (tran.gameplay.attackSweepRadiusScale < 0.1f) ? 0.1f : tran.gameplay.attackSweepRadiusScale;
    const float attackWidthScale = (tran.gameplay.attackWidthScale < 0.1f) ? 0.1f : tran.gameplay.attackWidthScale;
    const float attackDepthScale = (tran.gameplay.attackDepthScale < 0.1f) ? 0.1f : tran.gameplay.attackDepthScale;
    const float attackHitStop = (tran.gameplay.attackHitStop < 0.0f) ? 0.0f : tran.gameplay.attackHitStop;
    const float attackKnockback = (tran.gameplay.attackKnockback < 0.0f) ? 0.0f : tran.gameplay.attackKnockback;
    const float attackHitFlash = (tran.gameplay.attackHitFlash < 0.0f) ? 0.0f : tran.gameplay.attackHitFlash;
    const float attackTrailInterval = (tran.gameplay.attackTrailInterval < 0.0f) ? 0.0f : tran.gameplay.attackTrailInterval;
    const float attackTrailLife = (tran.gameplay.attackTrailLife < 0.0f) ? 0.0f : tran.gameplay.attackTrailLife;
    const float attackTrailScale = (tran.gameplay.attackTrailScale < 0.1f) ? 0.1f : tran.gameplay.attackTrailScale;
    const float playerDamageFlash = (tran.gameplay.playerDamageFlash < 0.0f) ? 0.0f : tran.gameplay.playerDamageFlash;
    const float playerDamageInvincible = (tran.gameplay.playerDamageInvincible < 0.0f) ? 0.0f : tran.gameplay.playerDamageInvincible;
    const float enemyDefeatFlash = (tran.gameplay.enemyDefeatFlash < 0.0f) ? 0.0f : tran.gameplay.enemyDefeatFlash;
    const float enemyDefeatFlashScale = (tran.gameplay.enemyDefeatFlashScale < 0.1f) ? 0.1f : tran.gameplay.enemyDefeatFlashScale;

    const float enemyAttackWindup = (tran.gameplay.enemyAttackWindup < 0.0f) ? 0.0f : tran.gameplay.enemyAttackWindup;
    const float enemyAttackCooldown = (tran.gameplay.enemyAttackCooldown < 0.0f) ? 0.0f : tran.gameplay.enemyAttackCooldown;
    const float enemyAttackRangeMin = (tran.gameplay.enemyAttackRangeMin < 0.1f) ? 0.1f : tran.gameplay.enemyAttackRangeMin;
    const float enemyAttackRangeScale = (tran.gameplay.enemyAttackRangeScale < 0.1f) ? 0.1f : tran.gameplay.enemyAttackRangeScale;
    const float enemyAttackDamageBase = (tran.gameplay.enemyAttackDamage < 0.0f) ? 0.0f : tran.gameplay.enemyAttackDamage;
    const float enemyMoveSpeedBase = (tran.gameplay.enemyMoveSpeed < 0.1f) ? 0.1f : tran.gameplay.enemyMoveSpeed;
    const float waveEnemyMoveSpeedAdd = (tran.gameplay.waveEnemyMoveSpeedAdd < 0.0f) ? 0.0f : tran.gameplay.waveEnemyMoveSpeedAdd;
    const float waveEnemyAttackDamageScale = (tran.gameplay.waveEnemyAttackDamageScalePerWave < 0.0f)
        ? 0.0f : tran.gameplay.waveEnemyAttackDamageScalePerWave;
    const float waveStep = static_cast<float>((m_currentWave > 0) ? (m_currentWave - 1) : 0);
    const float enemyAttackDamage =
        enemyAttackDamageBase *
        difficultyEnemyAttackDamageScale *
        roguelikeEnemyAttackScale *
        (1.0f + waveEnemyAttackDamageScale * waveStep);
    const float enemyMoveSpeed = enemyMoveSpeedBase + waveEnemyMoveSpeedAdd * waveStep;
    const float enemySeparationRadius = (tran.gameplay.enemySeparationRadius < 0.0f) ? 0.0f : tran.gameplay.enemySeparationRadius;
    const float enemySeparationWeight = (tran.gameplay.enemySeparationWeight < 0.0f) ? 0.0f : tran.gameplay.enemySeparationWeight;
    const float enemySeparationMaxOffset = (tran.gameplay.enemySeparationMaxOffset < 0.0f) ? 0.0f : tran.gameplay.enemySeparationMaxOffset;
    const float enemyProjectileSpeed = (tran.gameplay.enemyProjectileSpeed < 0.1f) ? 0.1f : tran.gameplay.enemyProjectileSpeed;
    const float enemyProjectileLife = (tran.gameplay.enemyProjectileLife < 0.05f) ? 0.05f : tran.gameplay.enemyProjectileLife;
    const float enemyProjectileRadius = (tran.gameplay.enemyProjectileRadius < 0.05f) ? 0.05f : tran.gameplay.enemyProjectileRadius;
    const float enemyProjectileDamageScale = (tran.gameplay.enemyProjectileDamageScale < 0.0f) ? 0.0f : tran.gameplay.enemyProjectileDamageScale;

    const float pushSlop = (tran.gameplay.pushSlop < 0.0f) ? 0.0f : tran.gameplay.pushSlop;
    float playerPushShare = Clamp01(tran.gameplay.playerPushShare);
    float enemyPushShare = Clamp01(tran.gameplay.enemyPushShare);
    const float pushShareSum = playerPushShare + enemyPushShare;
    if (pushShareSum <= 1.0e-6f)
    {
        playerPushShare = 0.5f;
        enemyPushShare = 0.5f;
    }
    else
    {
        playerPushShare /= pushShareSum;
        enemyPushShare /= pushShareSum;
    }

    tran.roguelike.attackPowerLevel = tran.ClampUpgradeLevel(tran.roguelike.attackPowerLevel);
    tran.roguelike.attackSpeedLevel = tran.ClampUpgradeLevel(tran.roguelike.attackSpeedLevel);
    tran.roguelike.evadeCooldownLevel = tran.ClampUpgradeLevel(tran.roguelike.evadeCooldownLevel);
    m_skill1CooldownDuration = skill1Cooldown;
    m_skill2CooldownDuration = skill2Cooldown;

    tran.gameplayDebug.effectiveEnemyBaseCount = effectiveBaseEnemyCount;
    tran.gameplayDebug.effectiveEnemyAddPerWave = effectiveWaveEnemyAdd;
    tran.gameplayDebug.effectiveEnemyAttackDamage = enemyAttackDamage;
    tran.gameplayDebug.playerAttackDamage = playerAttackDamage;
    tran.gameplayDebug.playerAttackCooldownScale = playerAttackCooldownScale;
    tran.gameplayDebug.playerEvadeCooldownScale = playerEvadeCooldownScale;
    tran.gameplayDebug.stageClearCount = tran.roguelike.stageClearCount;
    tran.gameplayDebug.attackPowerLevel = tran.roguelike.attackPowerLevel;
    tran.gameplayDebug.attackSpeedLevel = tran.roguelike.attackSpeedLevel;
    tran.gameplayDebug.evadeCooldownLevel = tran.roguelike.evadeCooldownLevel;
    tran.gameplayDebug.lastUpgradeType = tran.roguelike.lastUpgradeType;
    tran.gameplayDebug.upgradeSelectionPending = tran.roguelike.selectionPending;
    tran.gameplayDebug.upgradeRerollRemain = tran.roguelike.rerollRemain;
    tran.gameplayDebug.upgradeOffer0 = tran.roguelike.offers[0];
    tran.gameplayDebug.upgradeOffer1 = tran.roguelike.offers[1];
    tran.gameplayDebug.upgradeOffer2 = tran.roguelike.offers[2];

    if (!m_isBossBgmActive && m_currentWave >= m_waveMax && m_pBossBgm)
    {
        if (m_pGameBgmVoice)
        {
            m_pGameBgmVoice->Stop();
            m_pGameBgmVoice->DestroyVoice();
            m_pGameBgmVoice = nullptr;
        }
        m_pGameBgmVoice = PlaySound(m_pBossBgm);
        if (m_pGameBgmVoice)
        {
            m_isBossBgmActive = true;
        }
    }

    float stageSize = m_stageSize;
    if (tran.player.stageSize > 0.0f)
    {
        stageSize = tran.player.stageSize;
    }

    if (!m_isBossBattleDebug && waveEnemyTarget != m_requestedEnemyCount)
    {
        // Regular mode keeps the requested enemy count aligned with current wave rules.
        m_requestedEnemyCount = waveEnemyTarget;
        EnsureEnemyCount(m_requestedEnemyCount, stageSize);
    }
    else if (m_isBossBattleDebug && m_requestedEnemyCount != 0)
    {
        // Boss debug mode forces the normal enemy population to zero.
        m_requestedEnemyCount = 0;
        EnsureEnemyCount(0, stageSize);
    }

    const int nextMode = NormalizeCameraMode(tran.cameraMode);
    if (nextMode != m_cameraMode)
    {
        m_cameraMode = nextMode;
        m_pCamera = (m_cameraMode == kCameraModeDebug) ? static_cast<Camera*>(m_pCameraDebug)
            : static_cast<Camera*>(m_pCameraGame);
        if (m_pPlayer) m_pPlayer->SetCamera(m_pCamera);
        for (auto& slot : m_enemies)
        {
            if (slot.enemy) slot.enemy->SetCamera(m_pCamera);
        }
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

    const DirectX::XMFLOAT3 baseEye = tran.camera.eye;
    const DirectX::XMFLOAT3 baseLook = tran.camera.look;

    if (m_cameraMode == kCameraModeDebug)
    {
        tran.cameraDebug = tran.camera;
    }
    else
    {
        tran.cameraGame = tran.camera;
    }

    if (m_cameraIntroActive)
    {
        // The intro camera briefly pushes toward the player, then returns to the base pose.
        const float safeIntroDuration = (cameraIntroDuration > 0.0f) ? cameraIntroDuration : kFixedDt;
        m_cameraIntroTimer += kFixedDt;
        const float introT = Clamp01(m_cameraIntroTimer / safeIntroDuration);
        const bool returnPhase = (introT >= 0.5f);
        const float phaseT = returnPhase ? (introT - 0.5f) * 2.0f : introT * 2.0f;
        const float easedT = ExpEase01(phaseT);

        const DirectX::XMFLOAT3 introEye = returnPhase
            ? LerpFloat3(m_cameraIntroFocusEye, m_cameraIntroStartEye, easedT)
            : LerpFloat3(m_cameraIntroStartEye, m_cameraIntroFocusEye, easedT);
        const DirectX::XMFLOAT3 introLook = returnPhase
            ? LerpFloat3(m_cameraIntroFocusLook, m_cameraIntroStartLook, easedT)
            : LerpFloat3(m_cameraIntroStartLook, m_cameraIntroFocusLook, easedT);

        tran.camera.eye = introEye;
        tran.camera.look = introLook;
        if (m_cameraMode == kCameraModeDebug)
        {
            tran.cameraDebug = tran.camera;
            ApplyCameraPose(m_pCameraDebug, introEye, introLook);
        }
        else
        {
            tran.cameraGame = tran.camera;
            ApplyCameraPose(m_pCameraGame, introEye, introLook);
        }

        if (introT >= 1.0f)
        {
            m_cameraIntroActive = false;
            m_cameraIntroTimer = 0.0f;
            tran.camera.eye = m_cameraIntroStartEye;
            tran.camera.look = m_cameraIntroStartLook;
            if (m_cameraMode == kCameraModeDebug)
            {
                tran.cameraDebug = tran.camera;
                ApplyCameraPose(m_pCameraDebug, m_cameraIntroStartEye, m_cameraIntroStartLook);
            }
            else
            {
                tran.cameraGame = tran.camera;
                ApplyCameraPose(m_pCameraGame, m_cameraIntroStartEye, m_cameraIntroStartLook);
            }
        }

        tran.gameplayDebug.attackSwingId = m_attackSwingId;
        tran.gameplayDebug.swingHitCount = m_attackHitCountThisSwing;
        tran.gameplayDebug.attackActive = m_attackActive ? 1 : 0;
        tran.gameplayDebug.currentWave = m_currentWave;
        tran.gameplayDebug.maxWave = m_waveMax;
        tran.gameplayDebug.enemiesAlive = static_cast<int>(m_enemies.size());
        tran.gameplayDebug.enemiesTarget = m_requestedEnemyCount;
        UpdateHpGauge();
        UpdateCooldownGauges();
        m_uiManager.Update(UIObjectManager::Layer::Game);
        // During the intro, gameplay itself stays frozen after UI state is refreshed.
        return;
    }

    if (tran.gameplayDebug.runTimerRunning != 0)
    {
        tran.gameplayDebug.runElapsedSec += kFixedDt;
        if (tran.gameplayDebug.runElapsedSec < 0.0f)
        {
            tran.gameplayDebug.runElapsedSec = 0.0f;
        }
    }

    if (UpdateBossDebugSetup(stageSize))
    {
        return;
    }

    if (m_screenShakeTimer > 0.0f)
    {
        m_screenShakeTimer -= kFixedDt;
        if (m_screenShakeTimer < 0.0f) m_screenShakeTimer = 0.0f;
    }

    if (m_screenShakeTimer > 0.0f && m_cameraMode == kCameraModeGame && m_pCameraGame)
    {
        const float safeDuration = (m_screenShakeDuration > 0.0f) ? m_screenShakeDuration : kMultiHitShakeDurationDefault;
        const float t = Clamp01(m_screenShakeTimer / safeDuration);
        const float strength = m_screenShakeAmplitude * t;
        m_screenShakePhase += kFixedDt * 60.0f;
        const float shakeX = static_cast<float>(std::sin(m_screenShakePhase * 1.73f)) * strength;
        const float shakeY = static_cast<float>(std::cos(m_screenShakePhase * 2.41f)) * strength * 0.45f;

        DirectX::XMFLOAT3 shakenEye = baseEye;
        DirectX::XMFLOAT3 shakenLook = baseLook;
        shakenEye.x += shakeX;
        shakenEye.y += shakeY;
        shakenLook.x += shakeX;
        shakenLook.y += shakeY;
        tran.camera.eye = shakenEye;
        tran.camera.look = shakenLook;
        ApplyCameraPose(m_pCameraGame, shakenEye, shakenLook);
    }
    else
    {
        tran.camera.eye = baseEye;
        tran.camera.look = baseLook;
    }

    if (m_attackTrailSpawnTimer > 0.0f)
    {
        m_attackTrailSpawnTimer -= kFixedDt;
        if (m_attackTrailSpawnTimer < 0.0f) m_attackTrailSpawnTimer = 0.0f;
    }
    if (m_bossStompImpactTimer > 0.0f)
    {
        m_bossStompImpactTimer -= kFixedDt;
        if (m_bossStompImpactTimer < 0.0f) m_bossStompImpactTimer = 0.0f;
    }
    if (m_playerDamageFlashTimer > 0.0f)
    {
        m_playerDamageFlashTimer -= kFixedDt;
        if (m_playerDamageFlashTimer < 0.0f) m_playerDamageFlashTimer = 0.0f;
    }
    if (m_playerDamageInvincibleTimer > 0.0f)
    {
        m_playerDamageInvincibleTimer -= kFixedDt;
        if (m_playerDamageInvincibleTimer < 0.0f) m_playerDamageInvincibleTimer = 0.0f;
    }
    for (auto& fx : m_markerEffects)
    {
        if (fx.timer > 0.0f)
        {
            fx.timer -= kFixedDt;
            if (fx.timer < 0.0f) fx.timer = 0.0f;
        }
    }
    m_markerEffects.erase(
        std::remove_if(m_markerEffects.begin(), m_markerEffects.end(),
                       [](const MarkerEffect& fx) { return fx.timer <= 0.0f || fx.duration <= 0.0f; }),
        m_markerEffects.end());

    if (m_hitStopTimer > 0.0f)
    {
        m_hitStopTimer -= kFixedDt;
        if (m_hitStopTimer < 0.0f) m_hitStopTimer = 0.0f;
        tran.gameplayDebug.attackSwingId = m_attackSwingId;
        tran.gameplayDebug.swingHitCount = m_attackHitCountThisSwing;
        tran.gameplayDebug.attackActive = m_attackActive ? 1 : 0;
        tran.gameplayDebug.currentWave = m_currentWave;
        tran.gameplayDebug.maxWave = m_waveMax;
        tran.gameplayDebug.enemiesAlive = static_cast<int>(m_enemies.size());
        tran.gameplayDebug.enemiesTarget = m_requestedEnemyCount;
        UpdateHpGauge();
        UpdateCooldownGauges();
        m_uiManager.Update(UIObjectManager::Layer::Game);
        return;
    }

    if (m_attackCooldownUiTimer > 0.0f)
    {
        m_attackCooldownUiTimer -= kFixedDt;
        if (m_attackCooldownUiTimer < 0.0f) m_attackCooldownUiTimer = 0.0f;
    }
    if (m_skill1CooldownTimer > 0.0f)
    {
        m_skill1CooldownTimer -= kFixedDt;
        if (m_skill1CooldownTimer < 0.0f) m_skill1CooldownTimer = 0.0f;
    }
    if (m_skill2CooldownTimer > 0.0f)
    {
        m_skill2CooldownTimer -= kFixedDt;
        if (m_skill2CooldownTimer < 0.0f) m_skill2CooldownTimer = 0.0f;
    }
    if (m_skill1CooldownDuration <= 0.0f)
    {
        m_skill1CooldownTimer = 0.0f;
    }
    if (m_skill2CooldownDuration <= 0.0f)
    {
        m_skill2CooldownTimer = 0.0f;
    }
    if (m_skill1CooldownDuration > 0.0f && IsKeyTrigger('Q') && m_skill1CooldownTimer <= 0.0f)
    {
        // Skill body is not implemented yet; reserve cooldown behavior only.
        m_skill1CooldownTimer = m_skill1CooldownDuration;
    }
    if (m_skill2CooldownDuration > 0.0f && IsKeyTrigger('E') && m_skill2CooldownTimer <= 0.0f)
    {
        // Skill body is not implemented yet; reserve cooldown behavior only.
        m_skill2CooldownTimer = m_skill2CooldownDuration;
    }

    if (m_pPlayer) m_pPlayer->Update();
    const bool isPlayerEvading = (tran.gameplayDebug.playerEvading != 0);
    const auto commitLose = [&]()
    {
        tran.gameplayDebug.attackSwingId = m_attackSwingId;
        tran.gameplayDebug.swingHitCount = m_attackHitCountThisSwing;
        tran.gameplayDebug.attackActive = m_attackActive ? 1 : 0;
        tran.gameplayDebug.currentWave = m_currentWave;
        tran.gameplayDebug.maxWave = m_waveMax;
        tran.gameplayDebug.enemiesAlive = static_cast<int>(m_enemies.size());
        tran.gameplayDebug.enemiesTarget = m_requestedEnemyCount;
        tran.gameplayDebug.runTimerRunning = 0;
        tran.gameplayDebug.runRecordedSec = tran.gameplayDebug.runElapsedSec;
        tran.gameplayDebug.bossBattleActive = 0;
        tran.gameplayDebug.showBossResultTimer = 0;
        SceneManager::ChangeResult(SceneManager::ResultType::Lose);
        SceneManager::ChangeScene(SceneManager::SCENE_RESULT);
    };
    const auto applyPlayerDamage = [&](float damage) -> bool
    {
        if (damage <= 0.0f || isPlayerEvading || m_playerDamageInvincibleTimer > 0.0f)
        {
            return false;
        }
        if (m_pPlayerHitSe) PlaySound(m_pPlayerHitSe);
        tran.player.hp -= damage;
        if (m_playerDamageInvincibleTimer < playerDamageInvincible)
        {
            m_playerDamageInvincibleTimer = playerDamageInvincible;
        }
        if (m_playerDamageFlashTimer < playerDamageFlash)
        {
            m_playerDamageFlashTimer = playerDamageFlash;
        }
        if (tran.player.hp < 0.0f) tran.player.hp = 0.0f;
        if (tran.player.hp <= 0.0f)
        {
            commitLose();
            return true;
        }
        return false;
    };

    if (m_pPlayer && !m_enemyProjectiles.empty())
    {
        // Enemy projectiles are stepped first so their hits resolve before the rest of the melee update.
        Collision::Box playerBox = MakeAabb({
            tran.player.pos.x,
            tran.player.pos.y + tran.player.size.y * 0.5f,
            tran.player.pos.z
        }, tran.player.size);
        const float stageHalf = stageSize * 0.5f;
        const float outsideMargin = enemyProjectileRadius + 0.5f;

        for (auto& shot : m_enemyProjectiles)
        {
            if (shot.life <= 0.0f) continue;
            shot.pos.x += shot.vel.x * kFixedDt;
            shot.pos.y += shot.vel.y * kFixedDt;
            shot.pos.z += shot.vel.z * kFixedDt;
            shot.life -= kFixedDt;
            if (shot.life <= 0.0f)
            {
                shot.life = 0.0f;
                continue;
            }
            if (std::fabs(shot.pos.x) > stageHalf + outsideMargin ||
                std::fabs(shot.pos.z) > stageHalf + outsideMargin)
            {
                shot.life = 0.0f;
                continue;
            }

            if (!isPlayerEvading)
            {
                Collision::Box shotBox = MakeAabb(
                {
                    shot.pos.x,
                    shot.pos.y,
                    shot.pos.z
                },
                {
                    shot.radius * 2.0f,
                    shot.radius * 2.0f,
                    shot.radius * 2.0f
                });
                if (HitAabb(shotBox, playerBox))
                {
                    shot.life = 0.0f;
                    if (applyPlayerDamage(shot.damage))
                    {
                        return;
                    }
                }
            }
        }

        m_enemyProjectiles.erase(
            std::remove_if(m_enemyProjectiles.begin(), m_enemyProjectiles.end(),
                           [](const EnemyProjectile& shot) { return shot.life <= 0.0f; }),
            m_enemyProjectiles.end());
    }

    if (UpdateBossBattle(stageSize, playerAttackDamage, applyPlayerDamage))
    {
        // Boss update can end the run immediately, so stop further processing when it does.
        return;
    }

    const int enemyTotal = static_cast<int>(m_enemies.size());
    const bool heavyEnemyLoad = (enemyTotal >= 8);
    const bool runFullEnemyPairWork = !heavyEnemyLoad || ((m_enemyPerfPhase & 1u) == 0u);
    const int perfPhaseParity = static_cast<int>(m_enemyPerfPhase & 1u);
    ++m_enemyPerfPhase;
    const float separationRadiusSq = enemySeparationRadius * enemySeparationRadius;
    for (int i = 0; i < enemyTotal; ++i)
    {
        EnemySlot& slot = m_enemies[i];
        if (!slot.enemy) continue;

        // Enemy tuning is refreshed every frame so live ImGui edits take effect immediately.
        slot.enemy->SetSize(tran.player.size);
        slot.enemy->SetStageSize(stageSize);
        slot.enemy->SetMoveSpeed(enemyMoveSpeed);

        DirectX::XMFLOAT3 targetPos = m_pPlayer ? m_pPlayer->GetPos() : slot.enemy->GetPos();
        if (enemySeparationRadius > 0.0f && enemySeparationWeight > 0.0f)
        {
            const bool runSeparationThisEnemy = runFullEnemyPairWork || (((i + perfPhaseParity) & 1) == 0);
            if (runSeparationThisEnemy)
            {
                const Collision::Box selfBox = slot.enemy->GetCollision();
                DirectX::XMFLOAT3 separation = { 0.0f, 0.0f, 0.0f };
                int separationHitCount = 0;

                for (int j = 0; j < enemyTotal; ++j)
                {
                    if (j == i) continue;
                    Enemy* other = m_enemies[j].enemy;
                    if (!other) continue;

                    const Collision::Box otherBox = other->GetCollision();
                    const float dx = selfBox.center.x - otherBox.center.x;
                    const float dz = selfBox.center.z - otherBox.center.z;
                    if (std::fabs(dx) >= enemySeparationRadius || std::fabs(dz) >= enemySeparationRadius)
                    {
                        continue;
                    }

                    const float distSq = dx * dx + dz * dz;

                    if (distSq <= 1.0e-6f)
                    {
                        const float angle = (2.0f * kPi * static_cast<float>(i)) / static_cast<float>((enemyTotal > 0) ? enemyTotal : 1);
                        separation.x += std::cos(angle);
                        separation.z += std::sin(angle);
                        ++separationHitCount;
                    }
                    else if (distSq < separationRadiusSq)
                    {
                        const float dist = std::sqrt(distSq);
                        const float influence = 1.0f - (dist / enemySeparationRadius);
                        separation.x += (dx / dist) * influence;
                        separation.z += (dz / dist) * influence;
                        ++separationHitCount;
                    }

                    if (separationHitCount >= 4)
                    {
                        break;
                    }
                }

                const float sepLen = std::sqrt(separation.x * separation.x + separation.z * separation.z);
                if (sepLen > 1.0e-6f)
                {
                    DirectX::XMFLOAT3 sepDir = { separation.x / sepLen, 0.0f, separation.z / sepLen };
                    float sepOffset = enemySeparationWeight;
                    if (sepOffset > enemySeparationMaxOffset) sepOffset = enemySeparationMaxOffset;
                    targetPos.x += sepDir.x * sepOffset;
                    targetPos.z += sepDir.z * sepOffset;
                }
            }
        }

        slot.enemy->SetTargetPos(targetPos);
        slot.enemy->Update();
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

    if (m_attackCooldownTimer > 0.0f)
    {
        m_attackCooldownTimer -= kFixedDt;
        if (m_attackCooldownTimer < 0.0f) m_attackCooldownTimer = 0.0f;
    }

    if (m_attackRecoveryTimer > 0.0f)
    {
        m_attackRecoveryTimer -= kFixedDt;
        if (m_attackRecoveryTimer < 0.0f) m_attackRecoveryTimer = 0.0f;
    }
    if (m_enemyAttackSeGateTimer > 0.0f)
    {
        m_enemyAttackSeGateTimer -= kFixedDt;
        if (m_enemyAttackSeGateTimer < 0.0f) m_enemyAttackSeGateTimer = 0.0f;
    }

    if (IsKeyTrigger('F') &&
        !m_attackActive &&
        m_attackWindupTimer <= 0.0f &&
        m_attackRecoveryTimer <= 0.0f &&
        m_attackCooldownTimer <= 0.0f)
    {
        // Attack can only start when no other attack phase is still running.
        m_attackWindupTimer = attackWindup;
        m_attackCooldownUiDuration = attackWindup + attackDuration + attackRecovery + attackCooldown;
        if (m_attackCooldownUiDuration < kMinDuration) m_attackCooldownUiDuration = kMinDuration;
        m_attackCooldownUiTimer = m_attackCooldownUiDuration;
    }

    if (!m_attackActive && m_attackWindupTimer > 0.0f)
    {
        // Windup counts down first, then flips into the active hit window.
        m_attackWindupTimer -= kFixedDt;
        if (m_attackWindupTimer <= 0.0f)
        {
            m_attackWindupTimer = 0.0f;
            m_attackActive = true;
            m_attackTimer = attackDuration;
            ++m_attackSwingId;
            m_attackHitCountThisSwing = 0;
            if (m_attackSwingId < 0) m_attackSwingId = 0;
        }
    }

    if (m_attackActive)
    {
        // Active attack expires into recovery and cooldown.
        m_attackTimer -= kFixedDt;
        if (m_attackTimer <= 0.0f)
        {
            m_attackTimer = 0.0f;
            m_attackActive = false;
            m_attackRecoveryTimer = attackRecovery;
            m_attackCooldownTimer = attackCooldown;
        }
    }

    if (m_attackActive)
    {
        TRAN_INS;
        const DirectX::XMFLOAT3 forward = NormalizeXZ(m_lastMoveDir, { 0.0f, 0.0f, 1.0f });
        const DirectX::XMFLOAT3 right = { forward.z, 0.0f, -forward.x };
        const float progress = Clamp01(1.0f - (m_attackTimer / attackDuration));
        const float sweepRad = attackSweepDegrees * (kPi / 180.0f);
        const float angle = (progress - 0.5f) * sweepRad;
        const float c = std::cos(angle);
        const float s = std::sin(angle);
        DirectX::XMFLOAT3 slashDir = {
            (forward.x * c) + (right.x * s),
            0.0f,
            (forward.z * c) + (right.z * s)
        };
        slashDir = NormalizeXZ(slashDir, forward);

        m_attackSize = {
            tran.player.size.x * attackWidthScale,
            tran.player.size.y,
            tran.player.size.z * attackDepthScale
        };
        const float radius = tran.player.size.z * attackSweepRadiusScale;
        m_attackCenter = {
            tran.player.pos.x + slashDir.x * radius,
            tran.player.pos.y + tran.player.size.y * 0.5f,
            tran.player.pos.z + slashDir.z * radius
        };

        if (attackTrailLife > 0.0f)
        {
            if (attackTrailInterval <= 0.0f || m_attackTrailSpawnTimer <= 0.0f)
            {
                MarkerEffect fx{};
                fx.pos = m_attackCenter;
                fx.size = { m_attackSize.x * attackTrailScale, m_attackSize.y, m_attackSize.z * attackTrailScale };
                fx.color = { 1.0f, 0.35f, 0.20f, 0.50f };
                fx.timer = attackTrailLife;
                fx.duration = attackTrailLife;
                fx.growScale = 1.20f;
                if (m_markerEffects.size() < 96)
                {
                    m_markerEffects.push_back(fx);
                }
                m_attackTrailSpawnTimer = attackTrailInterval;
            }
        }
    }
    else
    {
        m_attackTrailSpawnTimer = 0.0f;
    }

    if (m_pPlayer && !m_enemies.empty())
    {
        TRAN_INS;

        Collision::Box playerBox = MakeAabb({
            tran.player.pos.x,
            tran.player.pos.y + tran.player.size.y * 0.5f,
            tran.player.pos.z
        }, tran.player.size);
        DirectX::XMFLOAT3 playerCenter = playerBox.center;
        bool pushedPlayer = false;
        const float stageHalf = stageSize * 0.5f;
        auto clampCenterToStage = [&](DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& size)
        {
            const float halfX = size.x * 0.5f;
            const float halfZ = size.z * 0.5f;
            float minX = -stageHalf + halfX;
            float maxX = stageHalf - halfX;
            float minZ = -stageHalf + halfZ;
            float maxZ = stageHalf - halfZ;
            if (minX > maxX) { minX = 0.0f; maxX = 0.0f; }
            if (minZ > maxZ) { minZ = 0.0f; maxZ = 0.0f; }
            center.x = ClampRange(center.x, minX, maxX);
            center.z = ClampRange(center.z, minZ, maxZ);
        };

        const int enemyCountNow = static_cast<int>(m_enemies.size());
        const int pairIStart = runFullEnemyPairWork ? 0 : perfPhaseParity;
        const int pairIStep = runFullEnemyPairWork ? 1 : 2;
        for (int i = pairIStart; i < enemyCountNow - 1; i += pairIStep)
        {
            Enemy* enemyA = m_enemies[i].enemy;
            if (!enemyA) continue;

            for (int j = i + 1; j < enemyCountNow; ++j)
            {
                Enemy* enemyB = m_enemies[j].enemy;
                if (!enemyB) continue;

                Collision::Box boxA = enemyA->GetCollision();
                Collision::Box boxB = enemyB->GetCollision();

                const float diffX = boxB.center.x - boxA.center.x;
                const float diffZ = boxB.center.z - boxA.center.z;
                const float overlapX = (boxA.size.x + boxB.size.x) * 0.5f - std::fabs(diffX);
                const float overlapZ = (boxA.size.z + boxB.size.z) * 0.5f - std::fabs(diffZ);
                if (overlapX <= 0.0f || overlapZ <= 0.0f)
                {
                    continue;
                }

                float pushDirX = 0.0f;
                float pushDirZ = 0.0f;
                float penetration = overlapX;
                if (overlapX <= overlapZ)
                {
                    pushDirX = (diffX >= 0.0f) ? 1.0f : -1.0f;
                    if (std::fabs(diffX) < 1.0e-4f)
                    {
                        pushDirX = ((i + j) & 1) ? -1.0f : 1.0f;
                    }
                }
                else
                {
                    pushDirZ = (diffZ >= 0.0f) ? 1.0f : -1.0f;
                    if (std::fabs(diffZ) < 1.0e-4f)
                    {
                        pushDirZ = ((i + j) & 1) ? -1.0f : 1.0f;
                    }
                    penetration = overlapZ;
                }

                const float pushDist = penetration + pushSlop;
                boxA.center.x -= pushDirX * pushDist * 0.5f;
                boxA.center.z -= pushDirZ * pushDist * 0.5f;
                boxB.center.x += pushDirX * pushDist * 0.5f;
                boxB.center.z += pushDirZ * pushDist * 0.5f;

                clampCenterToStage(boxA.center, boxA.size);
                clampCenterToStage(boxB.center, boxB.size);

                enemyA->SetPos({ boxA.center.x, 0.0f, boxA.center.z });
                enemyB->SetPos({ boxB.center.x, 0.0f, boxB.center.z });
            }
        }

        for (auto& slot : m_enemies)
        {
            if (!slot.enemy) continue;
            if (slot.hitFlashTimer > 0.0f)
            {
                slot.hitFlashTimer -= kFixedDt;
                if (slot.hitFlashTimer < 0.0f) slot.hitFlashTimer = 0.0f;
            }

            Collision::Box enemyBox = slot.enemy->GetCollision();

            // Resolve overlap with a simple push response so player/enemy do not stack.
            const float diffX = enemyBox.center.x - playerCenter.x;
            const float diffZ = enemyBox.center.z - playerCenter.z;
            const float overlapX = (playerBox.size.x + enemyBox.size.x) * 0.5f - std::fabs(diffX);
            const float overlapZ = (playerBox.size.z + enemyBox.size.z) * 0.5f - std::fabs(diffZ);
            if (!isPlayerEvading && overlapX > 0.0f && overlapZ > 0.0f)
            {
                float pushDirX = 0.0f;
                float pushDirZ = 0.0f;
                float penetration = overlapX;
                if (overlapX <= overlapZ)
                {
                    pushDirX = (diffX >= 0.0f) ? 1.0f : -1.0f;
                    if (std::fabs(diffX) < 1.0e-4f)
                    {
                        pushDirX = (m_lastMoveDir.x >= 0.0f) ? 1.0f : -1.0f;
                    }
                }
                else
                {
                    pushDirZ = (diffZ >= 0.0f) ? 1.0f : -1.0f;
                    if (std::fabs(diffZ) < 1.0e-4f)
                    {
                        pushDirZ = (m_lastMoveDir.z >= 0.0f) ? 1.0f : -1.0f;
                    }
                    penetration = overlapZ;
                }

                const float pushDist = penetration + pushSlop;
                playerCenter.x -= pushDirX * pushDist * playerPushShare;
                playerCenter.z -= pushDirZ * pushDist * playerPushShare;
                enemyBox.center.x += pushDirX * pushDist * enemyPushShare;
                enemyBox.center.z += pushDirZ * pushDist * enemyPushShare;

                clampCenterToStage(playerCenter, playerBox.size);
                clampCenterToStage(enemyBox.center, enemyBox.size);

                playerBox.center = playerCenter;
                slot.enemy->SetPos({ enemyBox.center.x, 0.0f, enemyBox.center.z });
                pushedPlayer = true;
            }

            enemyBox = slot.enemy->GetCollision();
            if (slot.attackCooldownTimer > 0.0f)
            {
                slot.attackCooldownTimer -= kFixedDt;
                if (slot.attackCooldownTimer < 0.0f) slot.attackCooldownTimer = 0.0f;
            }

            const float dx = enemyBox.center.x - playerBox.center.x;
            const float dz = enemyBox.center.z - playerBox.center.z;
            const float distSq = dx * dx + dz * dz;
            const float typeRangeScale = slot.enemy->GetAttackRangeScale();
            const float typeWindupScale = slot.enemy->GetAttackWindupScale();
            const float typeCooldownScale = slot.enemy->GetAttackCooldownScale();
            const float typeDamageScale = slot.enemy->GetAttackDamageScale();
            const bool isRangedEnemy = (slot.enemy->GetType() == static_cast<int>(Enemy::Type::Ranged));
            const float enemyRangeSize = (enemyBox.size.x > enemyBox.size.z) ? enemyBox.size.x : enemyBox.size.z;
            const float playerRangeSize = (playerBox.size.x > playerBox.size.z) ? playerBox.size.x : playerBox.size.z;
            float attackRange = (enemyRangeSize + playerRangeSize) * enemyAttackRangeScale * typeRangeScale;
            if (attackRange < enemyAttackRangeMin) attackRange = enemyAttackRangeMin;
            const bool inAttackRange = distSq <= attackRange * attackRange;
            slot.debugInAttackRange = inAttackRange;
            slot.debugAttackRange = attackRange;

            if (slot.attackWindupTimer > 0.0f)
            {
                slot.attackWindupTimer -= kFixedDt;
                if (slot.attackWindupTimer <= 0.0f)
                {
                    slot.attackWindupTimer = 0.0f;
                    slot.attackCooldownTimer = enemyAttackCooldown * typeCooldownScale;

                    if (inAttackRange)
                    {
                        if (isRangedEnemy)
                        {
                            DirectX::XMFLOAT3 dir = {
                                playerBox.center.x - enemyBox.center.x,
                                0.0f,
                                playerBox.center.z - enemyBox.center.z
                            };
                            dir = NormalizeXZ(dir, { 0.0f, 0.0f, 1.0f });

                            EnemyProjectile shot{};
                            shot.pos = {
                                enemyBox.center.x,
                                playerBox.center.y,
                                enemyBox.center.z
                            };
                            shot.vel = {
                                dir.x * enemyProjectileSpeed,
                                0.0f,
                                dir.z * enemyProjectileSpeed
                            };
                            shot.radius = enemyProjectileRadius;
                            shot.life = enemyProjectileLife;
                            shot.damage = enemyAttackDamage * typeDamageScale * enemyProjectileDamageScale;
                            if (m_enemyProjectiles.size() < 128)
                            {
                                m_enemyProjectiles.push_back(shot);
                            }
                        }
                        else if (applyPlayerDamage(enemyAttackDamage * typeDamageScale))
                        {
                            return;
                        }
                    }
                }
            }
            else if (slot.attackCooldownTimer <= 0.0f && inAttackRange)
            {
                slot.attackWindupTimer = enemyAttackWindup * typeWindupScale;
                if (m_pEnemyAttackSe && m_enemyAttackSeGateTimer <= 0.0f)
                {
                    PlaySound(m_pEnemyAttackSe);
                    m_enemyAttackSeGateTimer = 0.08f;
                }
            }

            if (m_attackActive && slot.lastHitSwingId != m_attackSwingId)
            {
                Collision::Box attackBox{};
                attackBox.center = m_attackCenter;
                attackBox.size = m_attackSize;
                if (HitAabb(attackBox, enemyBox))
                {
                    slot.enemy->Damage(playerAttackDamage);
                    slot.lastHitSwingId = m_attackSwingId;
                    ++m_attackHitCountThisSwing;
                    if (m_attackHitCountThisSwing == multiHitShakeThreshold &&
                        multiHitShakeDuration > 0.0f &&
                        multiHitShakeAmplitude > 0.0f)
                    {
                        m_screenShakeDuration = multiHitShakeDuration;
                        if (m_screenShakeTimer < multiHitShakeDuration)
                        {
                            m_screenShakeTimer = multiHitShakeDuration;
                        }
                        if (m_screenShakeAmplitude < multiHitShakeAmplitude)
                        {
                            m_screenShakeAmplitude = multiHitShakeAmplitude;
                        }
                        m_screenShakePhase = 0.0f;
                    }
                    slot.hitFlashTimer = attackHitFlash;
                    if (m_pAttackSe) PlaySound(m_pAttackSe);

                    const DirectX::XMFLOAT3 knockDir = NormalizeXZ({
                        enemyBox.center.x - m_attackCenter.x,
                        0.0f,
                        enemyBox.center.z - m_attackCenter.z
                    }, m_lastMoveDir);
                    enemyBox.center.x += knockDir.x * attackKnockback;
                    enemyBox.center.z += knockDir.z * attackKnockback;
                    clampCenterToStage(enemyBox.center, enemyBox.size);
                    slot.enemy->SetPos({ enemyBox.center.x, 0.0f, enemyBox.center.z });

                    if (m_hitStopTimer < attackHitStop)
                    {
                        m_hitStopTimer = attackHitStop;
                    }
                }
            }

            if (!slot.enemy->IsAlive())
            {
                if (enemyDefeatFlash > 0.0f)
                {
                    const Collision::Box deadBox = slot.enemy->GetCollision();
                    MarkerEffect fx{};
                    fx.pos = deadBox.center;
                    fx.size = {
                        deadBox.size.x * enemyDefeatFlashScale,
                        deadBox.size.y,
                        deadBox.size.z * enemyDefeatFlashScale
                    };
                    fx.color = { 1.0f, 0.80f, 0.20f, 0.95f };
                    fx.timer = enemyDefeatFlash;
                    fx.duration = enemyDefeatFlash;
                    fx.growScale = 1.35f;
                    if (m_markerEffects.size() < 96)
                    {
                        m_markerEffects.push_back(fx);
                    }
                }
                delete slot.enemy;
                slot.enemy = nullptr;
                slot.attackWindupTimer = 0.0f;
                slot.attackCooldownTimer = 0.0f;
                slot.debugInAttackRange = false;
                slot.debugAttackRange = 0.0f;
            }
        }

        if (pushedPlayer)
        {
            tran.player.pos.x = playerCenter.x;
            tran.player.pos.z = playerCenter.z;
            if (m_pPlayer)
            {
                m_pPlayer->SetPos({ playerCenter.x, tran.player.pos.y, playerCenter.z });
            }
        }

        m_enemies.erase(
            std::remove_if(m_enemies.begin(), m_enemies.end(), [](const EnemySlot& slot) { return slot.enemy == nullptr; }),
            m_enemies.end());

        if (!m_isBossBattleDebug && m_enemies.empty())
        {
            // Clearing all enemies advances the wave, or finishes the stage on the final wave.
            m_enemyProjectiles.clear();
            if (m_currentWave < m_waveMax)
            {
                ++m_currentWave;
                m_requestedEnemyCount = CalcWaveEnemyCount(effectiveBaseEnemyCount, m_currentWave, effectiveWaveEnemyAdd);
                EnsureEnemyCount(m_requestedEnemyCount, stageSize);
            }
            else
            {
                if (m_pClearSe) PlaySound(m_pClearSe);
                tran.gameplayDebug.attackSwingId = m_attackSwingId;
                tran.gameplayDebug.swingHitCount = m_attackHitCountThisSwing;
                tran.gameplayDebug.attackActive = m_attackActive ? 1 : 0;
                tran.gameplayDebug.currentWave = m_currentWave;
                tran.gameplayDebug.maxWave = m_waveMax;
                tran.gameplayDebug.enemiesAlive = static_cast<int>(m_enemies.size());
                tran.gameplayDebug.enemiesTarget = m_requestedEnemyCount;
                tran.gameplayDebug.runTimerRunning = 0;
                tran.gameplayDebug.runRecordedSec = tran.gameplayDebug.runElapsedSec;
                tran.gameplayDebug.bossBattleActive = 0;
                tran.gameplayDebug.showBossResultTimer = 0;
                tran.BeginUpgradeSelection();
                SceneManager::ChangeResult(SceneManager::ResultType::Win);
                SceneManager::ChangeScene(SceneManager::SCENE_RESULT);
                return;
            }
        }
    }

    {
        TRAN_INS;
        Enemy* trackedEnemy = nullptr;
        float nearestDistSq = 1.0e30f;

        // Publish the nearest enemy into Transfer for UI and direction-marker overlap checks.
        for (const auto& slot : m_enemies)
        {
            if (!slot.enemy) continue;
            const DirectX::XMFLOAT3 pos = slot.enemy->GetPos();
            const float dx = pos.x - tran.player.pos.x;
            const float dz = pos.z - tran.player.pos.z;
            const float distSq = dx * dx + dz * dz;
            if (!trackedEnemy || distSq < nearestDistSq)
            {
                trackedEnemy = slot.enemy;
                nearestDistSq = distSq;
            }
        }

        if (trackedEnemy)
        {
            tran.enemy.exists = 1;
            tran.enemy.pos = trackedEnemy->GetPos();
            tran.enemy.hp = static_cast<float>(trackedEnemy->GetHp());
            tran.enemy.maxHp = static_cast<float>(trackedEnemy->GetMaxHp());
            tran.enemy.state = trackedEnemy->GetState();
            tran.enemy.type = trackedEnemy->GetType();
        }
        else
        {
            tran.enemy.exists = 0;
            tran.enemy.pos = { 0.0f, 0.0f, 0.0f };
            tran.enemy.hp = 0.0f;
            tran.enemy.maxHp = 0.0f;
            tran.enemy.state = -1;
            tran.enemy.type = -1;
        }
    }

    tran.gameplayDebug.attackSwingId = m_attackSwingId;
    tran.gameplayDebug.swingHitCount = m_attackHitCountThisSwing;
    tran.gameplayDebug.attackActive = m_attackActive ? 1 : 0;
    tran.gameplayDebug.currentWave = m_currentWave;
    tran.gameplayDebug.maxWave = m_waveMax;
    tran.gameplayDebug.enemiesAlive = static_cast<int>(m_enemies.size());
    tran.gameplayDebug.enemiesTarget = m_requestedEnemyCount;
    tran.gameplayDebug.bossBattleActive = m_isBossBattleDebug ? 1 : 0;

    // Final UI state is refreshed after all gameplay mutations for the frame are complete.
    UpdateHpGauge();
    UpdateCooldownGauges();
    m_uiManager.Update(UIObjectManager::Layer::Game);
}

/**
 * @brief ゲームシーンの 3D と UI を描画します。
 */
void SceneGame::Draw()
{
    // カメラが無いと描画行列を作れないため何も描画しません。
    if (!m_pCamera) return;

    DirectX::XMFLOAT4X4 view = m_pCamera->GetViewMatrix();
    DirectX::XMFLOAT4X4 proj = m_pCamera->GetProjectionMatrix();

    // Geometry と Sprite の両方へ同じカメラ行列を設定します。
    Geometory::SetView(view);
    Geometory::SetProjection(proj);
    Sprite::SetView(view);
    Sprite::SetProjection(proj);

    SetDepthTest(true);

    float stage = m_stageSize;
    float groundTileSize = 1.0f;
    {
        TRAN_INS;
        if (tran.player.stageSize > 0.0f) stage = tran.player.stageSize;
        groundTileSize = tran.gameplay.groundTileSize;
    }

    if (m_pGroundTexture)
    {
        // The floor is drawn as tiled sprites so arbitrary stage sizes are covered without stretching.
        const float groundY = -0.001f;
        const float safeStage = (stage > 0.1f) ? stage : 0.1f;
        const float baseTileSize = ClampRange(groundTileSize, 0.5f, 10.0f);
        const int tileCountX = ClampInt(static_cast<int>(std::ceil(safeStage / baseTileSize)), 1, 64);
        const int tileCountZ = ClampInt(static_cast<int>(std::ceil(safeStage / baseTileSize)), 1, 64);
        const float stageHalf = safeStage * 0.5f;
        const DirectX::XMMATRIX rotate = DirectX::XMMatrixRotationX(DirectX::XM_PIDIV2);

        float cursorZ = -stageHalf;
        for (int z = 0; z < tileCountZ; ++z)
        {
            float tileDepth = baseTileSize;
            if (z == tileCountZ - 1)
            {
                tileDepth = safeStage - baseTileSize * static_cast<float>(tileCountZ - 1);
            }
            if (tileDepth <= 0.0f) continue;

            float cursorX = -stageHalf;
            for (int x = 0; x < tileCountX; ++x)
            {
                float tileWidth = baseTileSize;
                if (x == tileCountX - 1)
                {
                    tileWidth = safeStage - baseTileSize * static_cast<float>(tileCountX - 1);
                }
                if (tileWidth <= 0.0f) continue;

                const float centerX = cursorX + tileWidth * 0.5f;
                const float centerZ = cursorZ + tileDepth * 0.5f;
                DirectX::XMMATRIX translate = DirectX::XMMatrixTranslation(centerX, groundY, centerZ);
                DirectX::XMFLOAT4X4 world;
                DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranspose(rotate * translate));

                Sprite::SetWorld(world);
                Sprite::SetSize({ tileWidth, tileDepth });
                Sprite::SetOffset({ 0.0f, 0.0f });
                Sprite::SetUVPos({ 0.0f, 0.0f });
                Sprite::SetUVScale({ tileWidth / baseTileSize, tileDepth / baseTileSize });
                Sprite::SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
                Sprite::SetTexture(m_pGroundTexture);
                Sprite::Draw();

                cursorX += tileWidth;
            }
            cursorZ += tileDepth;
        }
    }

    SetDepthTest(false);

    if (m_attackActive)
    {
        // Active player swing is visualized as a floor marker.
        DrawAttackMarker(m_pAttackMarker, m_attackCenter, m_attackSize);
    }
    if (m_pAttackMarker)
    {
        for (const auto& shot : m_enemyProjectiles)
        {
            if (shot.life <= 0.0f) continue;
            const float size = shot.radius * 2.0f;
            DrawAttackMarkerTint(
                m_pAttackMarker,
                shot.pos,
                { size, size, size },
                kEnemyProjectileColor);
        }
    }
    DrawBossTelegraphMarker();

    static std::vector<DrawEntry> drawEntries;
    drawEntries.clear();
    const size_t drawEntryReserve = m_enemies.size() + 1;
    if (drawEntries.capacity() < drawEntryReserve)
    {
        drawEntries.reserve(drawEntryReserve);
    }
    DirectX::XMFLOAT3 cam = m_pCamera->GetPos();

    if (m_pPlayer)
    {
        TRAN_INS;
        const DirectX::XMFLOAT3 playerPos = {
            tran.player.pos.x,
            tran.player.pos.y + tran.player.size.y * 0.5f,
            tran.player.pos.z
        };
        const float dx = playerPos.x - cam.x;
        const float dy = playerPos.y - cam.y;
        const float dz = playerPos.z - cam.z;
        drawEntries.push_back({ dx * dx + dy * dy + dz * dz, true, false, playerPos, tran.player.size, nullptr });
    }
    AddBossDrawEntry(drawEntries, cam);

    for (auto& slot : m_enemies)
    {
        if (!slot.enemy) continue;
        const Collision::Box enemyBox = slot.enemy->GetCollision();
        const DirectX::XMFLOAT3 enemyPos = enemyBox.center;
        const DirectX::XMFLOAT3 enemySize = slot.enemy->GetSize();
        const float dx = enemyPos.x - cam.x;
        const float dy = enemyPos.y - cam.y;
        const float dz = enemyPos.z - cam.z;
        drawEntries.push_back({ dx * dx + dy * dy + dz * dz, false, false, enemyPos, enemySize, slot.enemy });
    }

    std::sort(drawEntries.begin(), drawEntries.end(), [](const DrawEntry& a, const DrawEntry& b)
    {
        return a.distSq > b.distSq;
    });

    // Transparent-ish billboard sprites are drawn back-to-front for more natural overlap.
    for (const auto& entry : drawEntries)
    {
        float shadowScale = 1.0f;
        if (entry.isBoss && m_bossStompImpactTimer > 0.0f)
        {
            const float impactRate = Clamp01(m_bossStompImpactTimer / kBossStompImpactFxDuration);
            shadowScale = 1.0f + (kBossStompImpactShadowScale - 1.0f) * impactRate;
        }
        DrawShadow(m_pShadow, entry.pos, entry.size, shadowScale);
        if (entry.isPlayer)
        {
            if (m_pPlayer) m_pPlayer->Draw();
        }
        else if (entry.isBoss)
        {
            DrawBossEntry(entry);
        }
        else if (entry.enemy)
        {
            entry.enemy->Draw();
        }
    }
    DrawBossFallingObjects();
    if (m_pPlayer)
    {
        m_pPlayer->DrawDirectionMarker();
    }

    if (m_pAttackMarker)
    {
        TRAN_INS;
        // Marker effects cover temporary hit flashes, defeat flashes, and player damage flashes.
        for (const auto& fx : m_markerEffects)
        {
            if (fx.timer <= 0.0f || fx.duration <= 0.0f) continue;
            const float t = Clamp01(fx.timer / fx.duration);
            const float growT = 1.0f + (1.0f - t) * (fx.growScale - 1.0f);
            DirectX::XMFLOAT4 color = fx.color;
            color.w *= t;
            DrawAttackMarkerTint(
                m_pAttackMarker,
                fx.pos,
                { fx.size.x * growT, fx.size.y, fx.size.z * growT },
                color);
        }

        const float flashDuration = (tran.gameplay.attackHitFlash > 0.0f) ? tran.gameplay.attackHitFlash : 0.001f;
        for (const auto& slot : m_enemies)
        {
            if (!slot.enemy || slot.hitFlashTimer <= 0.0f) continue;

            const Collision::Box enemyBox = slot.enemy->GetCollision();
            const float t = Clamp01(slot.hitFlashTimer / flashDuration);
            const float scale = 0.4f + t * 0.9f;
            DrawAttackMarkerTint(
                m_pAttackMarker,
                enemyBox.center,
                { enemyBox.size.x * scale, enemyBox.size.y, enemyBox.size.z * scale },
                { 1.0f, 0.35f, 0.25f, 0.80f * t + 0.10f });
        }

        const float playerFlashDuration = (tran.gameplay.playerDamageFlash > 0.0f) ? tran.gameplay.playerDamageFlash : 0.001f;
        const float playerFlashScale = (tran.gameplay.playerDamageFlashScale < 0.1f) ? 0.1f : tran.gameplay.playerDamageFlashScale;
        if (m_playerDamageFlashTimer > 0.0f)
        {
            const float t = Clamp01(m_playerDamageFlashTimer / playerFlashDuration);
            const float scale = playerFlashScale * (1.0f + (1.0f - t) * 0.25f);
            DrawAttackMarkerTint(
                m_pAttackMarker,
                {
                    tran.player.pos.x,
                    tran.player.pos.y + tran.player.size.y * 0.5f,
                    tran.player.pos.z
                },
                {
                    tran.player.size.x * scale,
                    tran.player.size.y,
                    tran.player.size.z * scale
                },
                { 1.0f, 0.1f, 0.1f, 0.55f * t + 0.10f });
        }
    }

    if (m_cameraMode == kCameraModeDebug)
    {
        TRAN_INS;
        // Debug mode overlays all important gameplay AABBs and attack ranges.
        Collision::Box playerBox{};
        playerBox.size = tran.player.size;
        playerBox.center = {
            tran.player.pos.x,
            tran.player.pos.y + tran.player.size.y * 0.5f,
            tran.player.pos.z
        };
        const float enemyAttackRangeMin = (tran.gameplay.enemyAttackRangeMin < 0.1f) ? 0.1f : tran.gameplay.enemyAttackRangeMin;
        const float enemyAttackRangeScale = (tran.gameplay.enemyAttackRangeScale < 0.1f) ? 0.1f : tran.gameplay.enemyAttackRangeScale;
        const bool showRangeDebug = true;
        if (m_pCameraGame)
        {
            AddCameraFrustumLines(*m_pCameraGame, { 1.0f, 1.0f, 0.0f, 1.0f });
        }

        AddAabbLines(playerBox, { 0.0f, 1.0f, 0.0f, 1.0f });

        for (const auto& slot : m_enemies)
        {
            if (!slot.enemy) continue;
            const Collision::Box enemyBox = slot.enemy->GetCollision();
            DirectX::XMFLOAT4 enemyAabbColor = kDebugEnemyColorOutRange;
            if (slot.hitFlashTimer > 0.0f)
            {
                enemyAabbColor = kDebugEnemyColorHit;
            }
            else if (slot.attackWindupTimer > 0.0f)
            {
                enemyAabbColor = kDebugEnemyColorWindup;
            }
            else if (slot.debugInAttackRange && slot.attackCooldownTimer <= 0.0f)
            {
                enemyAabbColor = kDebugEnemyColorReady;
            }
            else if (slot.debugInAttackRange)
            {
                enemyAabbColor = kDebugEnemyColorInRangeCooling;
            }
            AddAabbLines(enemyBox, enemyAabbColor);

            if (showRangeDebug)
            {
                float attackRange = slot.debugAttackRange;
                if (attackRange <= 0.0f)
                {
                    const float enemyRangeSize = (enemyBox.size.x > enemyBox.size.z) ? enemyBox.size.x : enemyBox.size.z;
                    const float playerRangeSize = (playerBox.size.x > playerBox.size.z) ? playerBox.size.x : playerBox.size.z;
                    attackRange = (enemyRangeSize + playerRangeSize) * enemyAttackRangeScale;
                    if (attackRange < enemyAttackRangeMin) attackRange = enemyAttackRangeMin;
                }

                Collision::Box attackRangeBox{};
                attackRangeBox.center = { enemyBox.center.x, playerBox.center.y, enemyBox.center.z };
                attackRangeBox.size = { attackRange * 2.0f, kDebugRangeBoxHeight, attackRange * 2.0f };
                const DirectX::XMFLOAT4 rangeColor = slot.debugInAttackRange ? kDebugRangeColorIn : kDebugRangeColorOut;
                AddAabbLines(attackRangeBox, rangeColor);
            }
        }

        for (const auto& shot : m_enemyProjectiles)
        {
            if (shot.life <= 0.0f) continue;
            Collision::Box shotBox{};
            shotBox.center = shot.pos;
            shotBox.size = { shot.radius * 2.0f, shot.radius * 2.0f, shot.radius * 2.0f };
            AddAabbLines(shotBox, { 0.25f, 0.95f, 1.0f, 1.0f });
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

    if (m_pEnemyHpFrame && m_pEnemyHpGauge && m_pCamera)
    {
        // Heavy enemy counts throttle HP billboard draws to reduce per-frame cost.
        const int enemyCount = static_cast<int>(m_enemies.size());
        const bool reduceHpBillboardLoad = (enemyCount >= 10);
        const int hpStep = reduceHpBillboardLoad ? 2 : 1;
        const int hpStart = reduceHpBillboardLoad ? static_cast<int>(m_enemyPerfPhase & 1u) : 0;
        for (int i = hpStart; i < enemyCount; i += hpStep)
        {
            const auto& slot = m_enemies[i];
            if (!slot.enemy) continue;
            const Collision::Box enemyBox = slot.enemy->GetCollision();
            const DirectX::XMFLOAT3 headPos = {
                enemyBox.center.x,
                enemyBox.center.y + enemyBox.size.y * kEnemyHpBillboardOffsetScale,
                enemyBox.center.z
            };

            float maxHp = static_cast<float>(slot.enemy->GetMaxHp());
            float hp = static_cast<float>(slot.enemy->GetHp());
            if (maxHp <= 0.0f) maxHp = 1.0f;
            const float rate = Clamp01(hp / maxHp);

            DrawEnemyHpGaugeBillboard(headPos, enemyBox.size, rate);
        }
    }

    m_uiManager.Draw(UIObjectManager::Layer::Game);
    DrawBossHpUi();
}

/**
 * @brief プレイヤー HP ゲージの位置と残量表示を更新します。
 */
void SceneGame::UpdateHpGauge()
{
    // UI 未生成時は更新先がないため何もしません。
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

    // ゲージは左詰め表示にするため、中心位置と UV 幅を残量に合わせて調整します。
    m_pHpGauge->SetPosition(gaugeX, gaugeY);
    m_pHpGauge->SetSize(gaugeWidth * rate, gaugeHeight);
    m_pHpGauge->SetUVPosition(0.0f, 0.0f);
    m_pHpGauge->SetUVScale(rate, 1.0f);
}

/**
 * @brief 攻撃・回避・スキルのクールタイムゲージを更新します。
 */
void SceneGame::UpdateCooldownGauges()
{
    float attackRate = 1.0f;
    if (m_attackCooldownUiDuration > 0.0f)
    {
        attackRate = Clamp01(1.0f - (m_attackCooldownUiTimer / m_attackCooldownUiDuration));
    }

    float evadeRate = 1.0f;
    if (m_pPlayer)
    {
        // 回避は Player 側の実効クールタイムを参照して進行率を出します。
        const float evadeDuration = m_pPlayer->GetEvadeCooldownDuration();
        const float evadeRemain = m_pPlayer->GetEvadeCooldownRemain();
        if (evadeDuration > 0.0f)
        {
            evadeRate = Clamp01(1.0f - (evadeRemain / evadeDuration));
        }
    }

    float skill1Rate = 1.0f;
    if (m_skill1CooldownDuration > 0.0f)
    {
        skill1Rate = Clamp01(1.0f - (m_skill1CooldownTimer / m_skill1CooldownDuration));
    }

    float skill2Rate = 1.0f;
    if (m_skill2CooldownDuration > 0.0f)
    {
        skill2Rate = Clamp01(1.0f - (m_skill2CooldownTimer / m_skill2CooldownDuration));
    }

    {
        TRAN_INS;
        // デバッグ UI からも見えるように、各ゲージ進行率を共有します。
        tran.gameplayDebug.cooldownRateAttack = attackRate;
        tran.gameplayDebug.cooldownRateEvade = evadeRate;
        tran.gameplayDebug.cooldownRateSkill1 = skill1Rate;
        tran.gameplayDebug.cooldownRateSkill2 = skill2Rate;
    }

    const float rates[CooldownSlotCount] = { attackRate, evadeRate, skill1Rate, skill2Rate };
    const float totalHeight =
        static_cast<float>(CooldownSlotCount) * kCooldownFrameHeight +
        static_cast<float>(CooldownSlotCount - 1) * kCooldownRowSpacing;
    const float frameX = static_cast<float>(SCREEN_WIDTH) - kUiMargin - kCooldownFrameWidth * 0.5f;
    const float startY = static_cast<float>(SCREEN_HEIGHT) - kUiMargin - totalHeight + kCooldownFrameHeight * 0.5f;
    const float gaugeWidth = kCooldownFrameWidth - kCooldownGaugePadding * 2.0f;
    const float gaugeHeight = kCooldownFrameHeight - kCooldownGaugePadding * 2.0f;

    for (int i = 0; i < CooldownSlotCount; ++i)
    {
        const float y = startY + static_cast<float>(i) * (kCooldownFrameHeight + kCooldownRowSpacing);
        if (m_pCooldownFrame[i])
        {
            m_pCooldownFrame[i]->SetPosition(frameX, y);
            m_pCooldownFrame[i]->SetSize(kCooldownFrameWidth, kCooldownFrameHeight);
        }
        if (m_pCooldownGauge[i])
        {
            // 各ゲージも HP ゲージと同様、左詰め表示になるよう中心を調整します。
            const float rate = Clamp01(rates[i]);
            const float gaugeLeft = frameX - kCooldownFrameWidth * 0.5f + kCooldownGaugePadding;
            const float gaugeX = gaugeLeft + gaugeWidth * rate * 0.5f;
            m_pCooldownGauge[i]->SetPosition(gaugeX, y);
            m_pCooldownGauge[i]->SetSize(gaugeWidth * rate, gaugeHeight);
            m_pCooldownGauge[i]->SetUVPosition(0.0f, 0.0f);
            m_pCooldownGauge[i]->SetUVScale(rate, 1.0f);
        }
    }
}

/**
 * @brief 敵の頭上にビルボード HP ゲージを描画します。
 * @param headPos 頭上基準位置です。
 * @param enemySize 敵サイズです。
 * @param rate 残 HP 比率です。
 */
void SceneGame::DrawEnemyHpGaugeBillboard(const DirectX::XMFLOAT3& headPos,
                                          const DirectX::XMFLOAT3& enemySize,
                                          float rate)
{
    // 必要な描画リソースかカメラが欠けている場合は描画しません。
    if (!m_pEnemyHpGauge || !m_pEnemyHpFrame || !m_pCamera) return;

    const float clampedRate = Clamp01(rate);
    float width = enemySize.x * kEnemyHpBillboardWidthScale;
    float height = enemySize.y * kEnemyHpBillboardHeightScale;
    if (width < kEnemyHpBillboardMinWidth) width = kEnemyHpBillboardMinWidth;
    if (height < kEnemyHpBillboardMinHeight) height = kEnemyHpBillboardMinHeight;

    // フレームとゲージの余白を敵サイズ比から決めます。
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
        // カメラ向きが壊れている場合は既定の前方で代用します。
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
        // 真上/真下に近い視線でも右軸を作れるよう、Up を差し替えます。
        up = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        right = DirectX::XMVector3Cross(up, forward);
    }
    right = DirectX::XMVector3Normalize(right);
    up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(forward, right));

    DirectX::XMFLOAT3 rightAxis;
    DirectX::XMStoreFloat3(&rightAxis, right);

    // 残量が減っても左端が固定されるよう、ゲージ中心だけを横へずらします。
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
        // 残量 0 より大きい時だけ中身ゲージを重ねます。
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
