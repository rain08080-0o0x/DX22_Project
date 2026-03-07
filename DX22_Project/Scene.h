
#pragma once

#include <array>
#include <list>
#include <vector>
#include "GameObject.h"

class Scene
{
protected:
	std::array<std::list<GameObject*>, 5> m_GameObject;
public:
	void RootUpdate();
	void RootDraw();

	Scene();
	virtual ~Scene();
	virtual void Init(){}
	virtual void Uninit(){}
	virtual void Update(){}
	virtual void Draw() {}

	void InitBase()
	{
		Init();
	}

	void UninitBase()
	{
		for (auto& objectList : m_GameObject)
		{
			for (GameObject* object : objectList)
			{
				object->Uninit();
				delete object;
			}
			objectList.clear();
		}

		Uninit();

	}

	void UpdateBase()
	{
		for (auto& objectList : m_GameObject)
		{
			for (GameObject* object : objectList)
			{
				object->UpDateBase();
			}
			objectList.remove_if([](GameObject* object) {return object->Destroy(); });
		}

		Update();
	}

	void DrawBase()
	{
		for (auto& objectList : m_GameObject)
		{
			for (GameObject* object : objectList)
			{
				object->DrawBase();
			}
		}
		Draw();
	}

	template<typename T>
	T* AddGameObject(int Layer = 0)
	{
		T* gameObject = new T();
		m_GameObject[Layer].push_back(gameObject);

		gameObject->init();

		return gameObject;
	}

	template<typename T>
	T* GetGameObject()
	{
		for (auto& objectList : m_GameObject)
		{
			for (GameObject* object : objectList)
			{
				if (typeid(*object) == typeid(T))
				{
					return (T*)object;
				}
			}
		}
		return nullptr;
	}

	template<typename T>
	std::vector<T*> GetGameObjects()
	{
		std::vector<T*> objects; // STL‚Ì”z—ñ
		for (auto& objectList : m_GameObject)
		{
			for (GameObject* object : objectList)
			{
				if (typeid(object) == typeid(T))
				{
					objects.push_back((T*)object);
				}
			}
		}
		return objects;
	}
};

