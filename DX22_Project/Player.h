#pragma once
#include "GameObject.h"
#include "Camera.h"
#include "Collision.h"
#include "Sprite.h"
#include "Texture.h"

class Player :
    public GameObject
{
public:
    //--- バウンド方向の定義 
    enum BoundAxis 
    {
        BoundX,
        BoundY,
        BoundZ,
    };
    enum eShotStep
    {
        SHOT_WAIT,  // 球を打つのを待つ 
        SHOT_KEEP,  // キー入力開始 
        SHOT_RELEASE, // キー入力をやめた（球を打つ 
    };
public:
    //--- 基本の処理 
    Player();
    ~Player();

    // 更新処理 
    void Update()override;

    // 描画処理 
    void Draw()override;
        // カメラの設定 
    void SetCamera(Camera* pCamera);
    Collision::Box GetCollision();
    void SetShadowPos(DirectX::XMFLOAT3 pos);

    Collision::Box GetShadowCollision();
    //--- 以下の関数の処理は後述 
public:
    void Bound(BoundAxis axis);
private:
    bool CheckStop();
    void UpdateShot();
    void UpdateMove();
private:
    //--- 各種メンバー変数 
    Camera* m_pCamera;  // ボールを追いかけるカメラの情報 
    DirectX::XMFLOAT3 m_move;   // ボールの移動速度 
    bool     m_isStop;  // ボールの停止判定 
    bool     m_isGround;  // 地面接地判定 
    int     m_shotStep;  // ボールの処理手順 
    float    m_shotPower;  // ボールの打ち出し強さ 
    eShotStep m_shotstep;

    Collision::Box m_collision;

    Texture* m_pShadowTex; // 影の見た目 
    DirectX::XMFLOAT3 m_shadowPos;  // 影の位置 
    Collision::Box  m_shadowCollision; //  

};

