
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

	}


//protected:
//	Fade* m_pFade;  // フェード処理クラス 
//	int  m_next;  // 切り替え先のシーン 
public:
	// シーンで実行するフェードクラスを設定 
	//void SetFade(Fade* fade) { m_pFade = fade; }

	//// 基本クラスでは、フェードアウトの終了を検知してシーンの切り替えを有効にする 
	//virtual bool IsChangeScene();

	//// 次の切り替え先シーンを取得 
	//int GetNext() { return m_next; }

	//// 切り替え先のシーンを設定 
	//void SetNext(int next);
};;

