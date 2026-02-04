#pragma once
#include "Scene.h"
#include <DirectXMath.h>
#include "Camera.h"

class Scene3DEditor :
    public Scene
{
public:
    Scene3DEditor();
    ~Scene3DEditor();

    void Update()override;
    void Draw()override;

public:
    struct Transform
    {
        DirectX::XMFLOAT3 pos;      // 中心座標
        DirectX::XMFLOAT3 rotate;   // 回転角
        DirectX::XMFLOAT3 scale;    // サイズ
        DirectX::XMFLOAT3 gpos;     // グローバル座標
        DirectX::XMFLOAT3 opos;     // オブジェクト座標
    };
private:
    Transform m_arm1;
    Transform m_arm2;
    Transform m_body;

    Camera* m_pCamera;
};

