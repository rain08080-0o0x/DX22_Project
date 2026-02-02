#ifndef __SCENE_GAME_H__
#define __SCENE_GAME_H__

#include "Scene.h"
#include "Camera.h"
#include "Player.h"
#include "UIObjectManager.h"

class UIObject;
class Enemy;

class SceneGame : public Scene
{
public:
    SceneGame();
    ~SceneGame();
    void Update() final;
    void Draw() final;

private:
    void UpdateHpGauge();

    Camera* m_pCamera;
    Player* m_pPlayer;
    Enemy* m_pEnemy;
    bool m_enemyWasOverlapping;
    UIObject* m_pHpFrame;
    UIObject* m_pHpGauge;
    UIObjectManager m_uiManager;
    float m_stageSize;
};

#endif // __SCENE_GAME_H__
