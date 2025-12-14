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
    ~Dice() {}

    // 一個分の初期化
    void Init(const DirectX::XMFLOAT3& pos, float size);

    // 毎フレーム処理
    void Update(float dt);

    // 描画
    void Draw();

    // 終了処理（今は何もしないが形だけ用意）
    void Uninit();

    const Collision::Box GetCollision(){ return m_box; }

    void SetCamera(Camera *set);
public:
    const DirectX::XMFLOAT3 GetPos();
    const DirectX::XMFLOAT3 GetVel();
    void SetVel(const DirectX::XMFLOAT3 v);

    void AddPos(const DirectX::XMFLOAT3 dp);

    float GetSize() const { return m_size; }
private:
    DirectX::XMFLOAT3 m_pos;   // 中心位置
    DirectX::XMFLOAT3 m_vel;   // 速度
    float             m_size;  // 一辺の長さ

    float m_mass;
    float m_restitution; // 反発係数 (0〜1)
    float m_friction;    // 地面との摩擦の強さ

    Collision::Box m_box; // 当たり判定用（AABB）

    Model *m_pModel;
    Camera* m_pCamera;
};
