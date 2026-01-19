#pragma once

#include "Scene.h"
#include "ECS.h"

class SceneGame : public Scene
{
public:
    SceneGame();
    ~SceneGame();
    void Update() final;
    void Draw() final;

private:
    void UpdatePlayerMode();
    void UpdateDiceMode();

private:
    ECS::World m_world;
    ECS::Entity m_gameEntity = ECS::kInvalidEntity;
};
