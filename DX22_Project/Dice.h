// Dice.h
#pragma once
#include <DirectXMath.h>
#include "Collision.h"
#include "Model.h"
#include "Camera.h"

class Dice
{
public:
    Dice();

    // 初期化：位置とサイズだけ決める
    void Init(const DirectX::XMFLOAT3& pos, float size);

    // 更新：移動と回転（止める処理は一切しない）
    void Update(float dt);

    // 描画：S * R * T
    void Draw();

    void Uninit() {}

    // 当たり判定（AABB）
    const Collision::Box& GetCollision() const { return m_box; }

    // 外から制御したい時用（RollAllで使う）
    void SetAngVel(const DirectX::XMFLOAT3& w) { m_angVel = w; }
    void SetRotation(const DirectX::XMFLOAT4& q) { m_rot = q; }

    const DirectX::XMFLOAT3& GetPos() const { return m_pos; }
    const DirectX::XMFLOAT4& GetRot() const { return m_rot; }
    void SetCamera(Camera* set) { m_pCamera = set; }
public:
    void ResetIsSleeping() { m_sleeping = false; m_sleepFrames = 0; }
    bool IsSleeping() const { return m_sleeping; }

    void WakeUp(); // MoveAllDice 等で呼ぶ
public:
    Collision::OBB GetOBB();
public:
    const DirectX::XMFLOAT3& GetVel() const { return m_vel; }
    void SetVel(const DirectX::XMFLOAT3& v) { m_vel = v; }

    void AddPos(const DirectX::XMFLOAT3& dp)
    {
        m_pos.x += dp.x;
        m_pos.y += dp.y;
        m_pos.z += dp.z;
    }
    void AddAngVel(const DirectX::XMFLOAT3& dw)
    {
        m_angVel.x += dw.x;
        m_angVel.y += dw.y;
        m_angVel.z += dw.z;
    }

private:
    // 状態（最小）
    DirectX::XMFLOAT3 m_pos;     // 中心位置
    DirectX::XMFLOAT3 m_vel;     // 速度（m/s）
    DirectX::XMFLOAT4 m_rot;     // 姿勢（クォータニオン）
    DirectX::XMFLOAT3 m_angVel;  // 角速度（rad/s）
    float m_size;                // 一辺

    // パラメータ（最小）
    float m_mass;

    // 当たり判定（AABB）
    Collision::Box m_box;

    // モデル
    Model* m_pModel;
    // カメラ
    Camera* m_pCamera;
private:
    bool m_sleeping = false;
    int  m_sleepFrames = 0;

private:
    static constexpr float GRAVITY = -9.8f;

    float m_restitution;   // 反発係数（0〜1）
    float m_linearDamping; // 空気抵抗（速度減衰）
    float m_friction;      // 接地時の摩擦

    int WALL_LIMIT_X;
    int WALL_LIMIT_Y;
    int WALL_LIMIT_Z;

private:
    // 内部ヘルパー：角速度で姿勢を積分
    void IntegrateRotation(float dt);
};