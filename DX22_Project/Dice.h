#pragma once
#include "GameObject.h"
#include "Camera.h"
#include "Collision.h"

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
};

