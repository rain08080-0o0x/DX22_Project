#pragma once
#include <DirectXMath.h>
#include "GameObject.h"
#include "Texture.h"
#include "Camera.h"

class Goal :
    public GameObject
{
private:
    Texture* m_pGoalTex;
    Camera* m_pCamera;
    DirectX::XMFLOAT3 m_scale;

public:
    Goal(DirectX::XMFLOAT3 size);
    ~Goal();

    void Update() override;
    void Draw() override;

    void SetCamera(Camera* camera);
};
