#pragma once
#include "GameObject.h"
#include "Texture.h"
#include "Sprite.h"
#include <DirectXMath.h>
#include "TrailEffect.h"
#include "Camera.h"

class TrailEffect;

class Player : public GameObject
{
public:
    Player(Camera*);
    ~Player();

    void Update() override;
    void Draw() override;

    void SetCamera(Camera*set);

private:
    void SyncFromTransfer();
    void SyncToTransfer();
    void ApplyMovement(float dt);
    void ClampToStage();


private:
    Camera* m_pCamera;
    Texture* m_pTexture;
    DirectX::XMFLOAT3 m_size;
    DirectX::XMFLOAT3 m_velocity;
    DirectX::XMFLOAT4 m_color;

    float m_hp;
    float m_maxHp;
    float m_moveSpeed;
    float m_dashDistance;
    float m_dashCooldown;
    float m_dashDuration;
    float m_dashTimer;
    float m_dashCooldownTimer;
    DirectX::XMFLOAT3 m_dashDir;
    bool m_isDashing;

    float m_stageSize;

    TrailEffect* m_pTrail;
    Texture* m_pTrailEffectTexture;
};