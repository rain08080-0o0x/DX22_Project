#pragma once
#include "GameObject.h"
#include "Texture.h"
#include "Sprite.h"
#include "Collision.h"
#include <DirectXMath.h>
#include "Camera.h"

class Enemy : public GameObject
{
public:
    enum class Type
    {
        Speed = 0,
        Tank = 1,
        Ranged = 2
    };

    Enemy();
    ~Enemy();

    void Update() override;
    void Draw() override;

    void SetSize(const DirectX::XMFLOAT3& size);
    void SetStageSize(float size);
    void SetMoveSpeed(float speed);
    void SetType(Type type);
    void SetHpScale(float scale);
    DirectX::XMFLOAT3 GetSize() const;
    Collision::Box GetCollision() const;
    void Damage(int amount);
    bool IsAlive() const;
    int GetHp() const;
    int GetMaxHp() const;
    int GetState() const;
    int GetType() const;
    float GetAttackRangeScale() const;
    float GetAttackWindupScale() const;
    float GetAttackCooldownScale() const;
    float GetAttackDamageScale() const;
    void SetCamera(Camera*);

    void SetTargetPos(DirectX::XMFLOAT3);
private:
    enum class MoveState
    {
        Wander,
        Chase
    };

    Camera* m_pCamera;
    Texture* m_pTexture;
    DirectX::XMFLOAT3 m_size;
    DirectX::XMFLOAT4 m_color;
    float m_moveSpeed;
    float m_moveSpeedBase;
    float m_moveSpeedScale;
    float m_stageSize;
    float m_moveDirX;
    int m_hp;
    int m_maxHp;
    Type m_type;
    float m_attackRangeScale;
    float m_attackWindupScale;
    float m_attackCooldownScale;
    float m_attackDamageScale;
    DirectX::XMFLOAT3 m_targetPos;
    DirectX::XMFLOAT3 m_wanderTarget;
    float m_wanderTimer;
    MoveState m_state;
};
