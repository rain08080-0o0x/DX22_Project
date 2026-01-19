#pragma once
#include "SceneManager.h"
#include "ECS.h"

class SceneResult : public Scene
{
public:
	SceneResult();
	~SceneResult();
	void Update() final;
	void Draw() final;
private:
	SceneManager::ResultType m_current;
	ECS::World m_world;
	ECS::Entity m_winner = ECS::kInvalidEntity;
	ECS::Entity m_loser = ECS::kInvalidEntity;
};

