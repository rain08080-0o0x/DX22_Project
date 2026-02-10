#pragma once
#include "SceneManager.h"
#include "UIObject.h"

class SceneResult :
    public Scene
{
public:
	SceneResult();
	~SceneResult();
	void Update() final;
	void Draw() final;
private:
	UIObject* m_pWinner;
	UIObject* m_pLoser;
	SceneManager::ResultType m_current;
};

