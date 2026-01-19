#pragma once
#include "Scene.h"
#include "ECS.h"

class SceneTitle : public Scene
{
public:
    SceneTitle();
    ~SceneTitle();

    void Update() override;
    void Draw() override;

private:
    ECS::World m_world;
    ECS::Entity m_logo = ECS::kInvalidEntity;
    ECS::Entity m_start = ECS::kInvalidEntity;
    ECS::Entity m_hint = ECS::kInvalidEntity;
};
