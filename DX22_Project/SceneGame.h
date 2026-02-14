#ifndef __SCENE_GAME_H__
#define __SCENE_GAME_H__

#include "Scene.h"
#include "Camera.h"
#include "Player.h"
#include "UIObjectManager.h"
#include "Goal.h"
#include <vector>

class UIObject;
class Enemy;
class Texture;
class CameraDebug;
struct XAUDIO2_BUFFER;
struct IXAudio2SourceVoice;

class SceneGame : public Scene
{
public:
    SceneGame();
    ~SceneGame();
    void Update() final;
    void Draw() final;

private:
    struct EnemySlot
    {
        Enemy* enemy = nullptr;
        float attackWindupTimer = 0.0f;
        float attackCooldownTimer = 0.0f;
        int lastHitSwingId = -1;
        float hitFlashTimer = 0.0f;
        bool debugInAttackRange = false;
        float debugAttackRange = 0.0f;
    };
    struct MarkerEffect
    {
        DirectX::XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 size = { 1.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
        float timer = 0.0f;
        float duration = 0.0f;
        float growScale = 1.0f;
    };

    void UpdateHpGauge();
    void SpawnEnemyByIndex(int index, float stageSize);
    void EnsureEnemyCount(int targetCount, float stageSize);
    int CalcWaveEnemyCount(int baseCount, int waveIndex, int addPerWave) const;
    void DrawEnemyHpGaugeBillboard(const DirectX::XMFLOAT3& headPos,
                                   const DirectX::XMFLOAT3& enemySize,
                                   float rate);

    Camera* m_pCamera;
    CameraDebug* m_pCameraGame;
    CameraDebug* m_pCameraDebug;
    int m_cameraMode;
    Player* m_pPlayer;
    std::vector<EnemySlot> m_enemies;
    std::vector<MarkerEffect> m_markerEffects;
    Texture* m_pShadow;
    Texture* m_pAttackMarker;
    XAUDIO2_BUFFER* m_pAttackSe;
    XAUDIO2_BUFFER* m_pPlayerHitSe;
    XAUDIO2_BUFFER* m_pEnemyAttackSe;
    XAUDIO2_BUFFER* m_pClearSe;
    XAUDIO2_BUFFER* m_pGameBgm;
    XAUDIO2_BUFFER* m_pBossBgm;
    IXAudio2SourceVoice* m_pGameBgmVoice;
    bool m_isBossBgmActive;
    Texture* m_pEnemyHpFrame;
    Texture* m_pEnemyHpGauge;
    UIObject* m_pHpFrame;
    UIObject* m_pHpGauge;
    UIObjectManager m_uiManager;
    float m_stageSize;
    int m_requestedEnemyCount;
    int m_currentWave;
    int m_waveMax;

    bool m_attackActive;
    float m_attackTimer;
    float m_attackWindupTimer;
    float m_attackRecoveryTimer;
    float m_attackCooldownTimer;
    int m_attackSwingId;
    int m_attackHitCountThisSwing;
    float m_hitStopTimer;
    float m_attackTrailSpawnTimer;
    float m_playerDamageFlashTimer;
    float m_enemyAttackSeGateTimer;
    unsigned int m_enemyPerfPhase;
    DirectX::XMFLOAT3 m_lastMoveDir;
    DirectX::XMFLOAT3 m_attackCenter;
    DirectX::XMFLOAT3 m_attackSize;
    Goal* m_pGoal;
};

#endif // __SCENE_GAME_H__

