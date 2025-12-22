#pragma once
#include "GameObject.h"
#include "Camera.h"
#include "Collision.h"
#include "Transfer.h"
#include "Model.h"

class Dice
    :public GameObject
{
public:
    //--- 基本の処理 
    Dice();
    ~Dice();

    // 更新処理 
    void Update()override;

    // 描画処理 
    void Draw()override;
    void TestUpdate();
    void TestDraw();

    // カメラの設定 
    void SetCamera(Camera* pCamera);
    Collision::Box GetCollision();

    void Roll();

    DirectX::XMFLOAT3 GetPos() { return m_pos; }

private:
    //--- 各種メンバー変数 
    Camera* m_pCamera;  // ボールを追いかけるカメラの情報 
    DirectX::XMFLOAT3 m_velocity;   // ボールの移動速度 
    bool     m_isGround;  // 地面接地判定 

    DirectX::XMFLOAT2 m_wallpos;    //壁の中心位置
    DirectX::XMFLOAT2 m_wallsize;   //壁のサイズ

    DirectX::XMFLOAT3 m_size;

    Collision::Box m_collision;

    // Transferのgroundを使うためのインスタンス
    Transfer& tran;

    Model* m_pModel;
private:    // 物理演算用変数宣言
    DirectX::XMFLOAT4 m_rot;        // 回転（クォータニオン） (x,y,z,w)
    DirectX::XMFLOAT3 m_angVel;     // 角速度（rad/frame想定）
    float m_mass;                   // 質量（とりあえず 1.0）
    float m_restitution;            // 反発係数
    float m_mu;                     // 動摩擦係数
private: //テスト用行列配列
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 obj;
    DirectX::XMFLOAT3 vertex[8];
};

