#ifndef __SCENE_GAME_H__
#define __SCENE_GAME_H__

#include "Scene.h"
#include "Camera.h"
#include "Player.h"
#include "UIObjectManager.h"
#include "Goal.h"

class UIObject;
class Enemy;
class Texture;
class CameraDebug;

class SceneGame : public Scene
{
public:
    SceneGame();
    ~SceneGame();
    void Update() final;
    void Draw() final;

private:
    void UpdateHpGauge();
    void DrawEnemyHpGaugeBillboard(const DirectX::XMFLOAT3& headPos,
                                   const DirectX::XMFLOAT3& enemySize,
                                   float rate);

    Camera* m_pCamera;
    CameraDebug* m_pCameraGame;
    CameraDebug* m_pCameraDebug;
    int m_cameraMode;
    Player* m_pPlayer;
    Enemy* m_pEnemy;
    bool m_enemyWasOverlapping;
    Texture* m_pShadow;
    Texture* m_pAttackMarker;
    Texture* m_pEnemyHpFrame;
    Texture* m_pEnemyHpGauge;
    UIObject* m_pHpFrame;
    UIObject* m_pHpGauge;
    UIObjectManager m_uiManager;
    float m_stageSize;

    bool m_attackActive;
    float m_attackTimer;
    bool m_attackHitThisSwing;
    DirectX::XMFLOAT3 m_lastMoveDir;
    DirectX::XMFLOAT3 m_attackCenter;
    DirectX::XMFLOAT3 m_attackSize;
    Goal* m_pGoal;
};

#endif // __SCENE_GAME_H__

