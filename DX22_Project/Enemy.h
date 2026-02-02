#pragma once
#include "GameObject.h"
#include "Texture.h"
#include "Sprite.h"
#include "Collision.h"
#include <DirectXMath.h>

class Enemy : public GameObject
{
public:
    Enemy();
    ~Enemy();

    void Update() override;
    void Draw() override;

    void SetSize(const DirectX::XMFLOAT3& size);
    DirectX::XMFLOAT3 GetSize() const;
    Collision::Box GetCollision() const;

private:
    Texture* m_pTexture;
    DirectX::XMFLOAT3 m_size;
    DirectX::XMFLOAT4 m_color;
};
