#pragma once
#include "Scene.h"

class Manager
{
private:
	static class Scene* m_pScene;
public:
	static void Init();
	static void Uninit();
	static void Update();
	static void Draw();

	static class Scene* GetScene() { return m_pScene; }

	template<typename T>
	static void SetScene()
	{
		if (m_pScene)
		{
			m_pScene->UninitBase();
			delete m_pScene;
		}

		m_pScene = new T();
		m_pScene->InitBase();
	}

};

