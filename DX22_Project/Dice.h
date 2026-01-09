#pragma once
#include "GameObject.h"
#include "Camera.h"
#include "Collision.h"
#include "Transfer.h"
#include "Model.h"
#include "Defines.h"
#include <memory>


// 最大サイコロ数
const int MAX_DICE = 8;

class Dice
{
public:
    Dice();
    ~Dice();
    void Update(int);
    void Update(float dt = fFPS);   // 物理は body.Update(dt) だけ
    void Draw();

    void SetCamera(Camera* pCamera);

    // 物理への参照が必要なら
//  RigidBodyOBB Body() { return body; }
//  const RigidBodyOBB& Body() const { return body; }

    void TestUpdate();
    void TestDraw();
private:
    Camera* m_pCamera = nullptr;
    Model* m_pModel = nullptr;

    DirectX::XMFLOAT3 m_pos;    // 位置情報
    DirectX::XMFLOAT3 m_size;   // サイズ
    float m_mass;               // 質量

    DirectX::XMFLOAT3 vertex[8];
    // 物理本体
    RigidBodyOBB *body[MAX_DICE];

    // 表示用（必要なら）色などだけ Dice が持つ
    DirectX::XMFLOAT4 color = { 1,1,1,1 };


    // 物理ボディ配列（Dice同士の総当たり判定用）
    static constexpr int kMaxBodies = 16;
    RigidBodyOBB *m_bodies[kMaxBodies];
    int m_count = 0; // 実際に使ってる個数

};


